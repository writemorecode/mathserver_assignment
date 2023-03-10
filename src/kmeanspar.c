#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define THREAD_COUNT 4

typedef struct point
{
    float x;
    float y;
    size_t cluster;
} point_t;

typedef struct point_array
{
    point_t *data;
    size_t size;
    size_t capacity;
} point_array_t;

typedef struct thread_arg
{
    size_t thread_id;

    size_t point_index_start;
    size_t point_index_end;

    size_t cluster_index_start;
    size_t cluster_index_end;

    point_array_t *points;
    point_array_t *clusters;

    pthread_barrier_t *barrier;
    pthread_mutex_t *mutex;

    size_t *finished_threads;
} thread_arg_t;



point_array_t *read_data(char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: Could not open file '%s'.\n", filename);
        return NULL;
    }

    point_array_t *points = calloc(1, sizeof(point_array_t));
    if (points == NULL)
    {
        return NULL;
    }
    points->capacity = 2048;
    points->size = 0;
    points->data = calloc(points->capacity, sizeof(point_t));
    if (points->data == NULL)
    {
        free(points);
        return NULL;
    }

    char line[256] = {0};
    char *p_line;
    point_t *p_point;
    int i = 0;
    while ((p_line = fgets(line, 256, fp)) != NULL)
    {
        if (points->capacity == i)
        {
            p_point = realloc(points->data, sizeof(point_t) * points->capacity * 2);
            if (p_point == NULL)
            {
                free(points->data);
                return NULL;
            }
            else if (points->data != p_point)
            {
                points->data = p_point;
            }
            p_point = NULL;
            points->capacity *= 2;
        }
        sscanf(line, "%f %f", &points->data[i].x, &points->data[i].y);
        points->data[i].cluster = 0;
        i++;
    }
    fclose(fp);
    points->size = i;

    if(points->size == 0)
    {
        free(points->data);
        free(points);
        fprintf(stderr, "Error: Input file '%s' did not contain any points.\n", filename);
        points = NULL;
    }

    return points;
}

point_array_t *init_clusters(point_array_t *points, size_t k)
{
    point_array_t *clusters = calloc(1, sizeof(point_array_t));
    if (clusters == NULL)
    {
        return NULL;
    }
    clusters->capacity = k;
    clusters->size = k;
    clusters->data = calloc(clusters->capacity, sizeof(point_t));
    if (clusters->data == NULL)
    {
        free(clusters);
        return NULL;
    }

    srand(time(NULL));

    int r;
    for (size_t i = 0; i < k; i++)
    {
        r = rand() % points->size;
        clusters->data[i].x = points->data[r].x;
        clusters->data[i].y = points->data[r].y;
    }

    return clusters;
}

void update_cluster_centroids(point_array_t *points, point_array_t *clusters,
                              size_t p_start, size_t p_end, size_t c_start, size_t c_end)
{
    size_t *count = calloc(clusters->size, sizeof(size_t));
    if (count == NULL)
    {
        return;
    }

    point_t *temp = calloc(clusters->size, sizeof(point_t));
    if (temp == NULL)
    {
        return;
    }

    size_t c;
    for (size_t i = p_start; i < p_end; i++)
    {
        c = points->data[i].cluster;
        count[c]++;
        temp[c].x += points->data[i].x;
        temp[c].y += points->data[i].y;
    }
    for (size_t i = c_start; i < c_end; i++)
    {
        clusters->data[i].x = temp[i].x / count[i];
        clusters->data[i].y = temp[i].y / count[i];
    }

    free(count);
    free(temp);
}

size_t get_closest_centroid(point_t *point, point_array_t *clusters)
{
    size_t nearest_cluster = 0;
    double x_dist, y_dist, dist;
    double min_dist = INT_MAX;

    for (size_t c = 0; c < clusters->size; c++)
    {
        x_dist = point->x - clusters->data[c].x;
        y_dist = point->y - clusters->data[c].y;

        dist = x_dist * x_dist + y_dist * y_dist;
        if (dist < min_dist)
        {
            min_dist = dist;
            nearest_cluster = c;
        }
    }
    return nearest_cluster;
}

bool assign_clusters_to_points(point_array_t *points, point_array_t *clusters,
                               size_t p_start, size_t p_end)
{
    size_t old_cluster, new_cluster;
    bool finished = true;
    for (size_t i = p_start; i < p_end; i++)
    {
        old_cluster = points->data[i].cluster;
        new_cluster = get_closest_centroid(&points->data[i], clusters);
        if (old_cluster != new_cluster)
        {
            finished = false;
            points->data[i].cluster = new_cluster;
        }
    }
    return finished;
}

/*
    Given points a,b in R2, returns ||a-b||^2.
*/
double point_distance(point_t *a, point_t *b)
{
    if (a == NULL || b == NULL)
    {
        return -1;
    }
    double x_dist = a->x - b->x;
    double y_dist = a->y - b->y;
    return x_dist * x_dist + y_dist * y_dist;
}

void barrier(pthread_mutex_t *m, pthread_barrier_t *b, size_t *finished_threads)
{
    pthread_barrier_wait(b);
    pthread_mutex_lock(m);
    if(*finished_threads == THREAD_COUNT)
    {
        pthread_mutex_unlock(m);
        pthread_exit(NULL);
    }
    *finished_threads = 0;
    pthread_mutex_unlock(m);
    pthread_barrier_wait(b);
}

void *worker(void *arg)
{
    thread_arg_t *thr_arg = (thread_arg_t *)arg;
    point_array_t *points = thr_arg->points;
    point_array_t *clusters = thr_arg->clusters;
    bool thread_finished = false;

    while (*thr_arg->finished_threads < 4)
    {
        thread_finished = assign_clusters_to_points(points, clusters, thr_arg->point_index_start,
                                                    thr_arg->point_index_end);
        pthread_barrier_wait(thr_arg->barrier);

        if(thread_finished)
        {
            pthread_mutex_lock(thr_arg->mutex);
            *thr_arg->finished_threads += 1;
            pthread_mutex_unlock(thr_arg->mutex);
        }

        barrier(thr_arg->mutex, thr_arg->barrier, thr_arg->finished_threads);

        update_cluster_centroids(points, clusters, thr_arg->point_index_start,
                                 thr_arg->point_index_end, thr_arg->cluster_index_start,
                                 thr_arg->cluster_index_end);

        barrier(thr_arg->mutex, thr_arg->barrier, thr_arg->finished_threads);
    }

    return NULL;
}

void run(point_array_t *points, point_array_t *clusters)
{
    pthread_t threads[THREAD_COUNT] = {0};
    thread_arg_t *args = calloc(THREAD_COUNT, sizeof(thread_arg_t));
    if (args == NULL)
    {
        perror("calloc");
        return;
    }

    size_t points_per_thread = points->size / THREAD_COUNT;
    size_t points_remainder = points->size % THREAD_COUNT;
    size_t clusters_per_thread = clusters->size / THREAD_COUNT;
    size_t clusters_remainder = clusters->size % THREAD_COUNT;

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, THREAD_COUNT);

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    size_t finished_threads = 0;

    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        args[i].points = points;
        args[i].clusters = clusters;
        args[i].point_index_start = points_per_thread * i;
        args[i].point_index_end = points_per_thread * (i + 1);
        args[i].cluster_index_start = clusters_per_thread * i;
        args[i].cluster_index_end = clusters_per_thread * (i + 1);
        args[i].thread_id = i;
        args[i].barrier = &barrier;
        args[i].mutex = &mutex;
        args[i].finished_threads = &finished_threads;
    }

    if (points_remainder > 0)
    {
        args[THREAD_COUNT - 1].point_index_end += points_remainder;
    }

    if (clusters_remainder > 0)
    {
        args[THREAD_COUNT - 1].cluster_index_end += clusters_remainder;
    }

    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        pthread_create(&threads[i], NULL, worker, &args[i]);
    }

    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(threads[i], NULL);
    }

    free(args);
}

void write_results(point_array_t *points)
{
    for (size_t i = 0; i < points->size; i++)
    {
        fprintf(stdout, "%.2f %.2f %ld\n", points->data[i].x, points->data[i].y, points->data[i].cluster);
    }
}

int main(int argc, char **argv)
{
    char *filename = NULL;
    size_t k = 9;

    int opt;
    while ((opt = getopt(argc, argv, "f:k:")) != -1)
    {
        if (opt == 'f')
        {
            filename = optarg;
        }
        else if (opt == 'k')
        {
            k = atoi(optarg);
            if (k < 0)
            {
                fprintf(stderr, "Error: Number of clusters cannot be negative!\n");
                return EXIT_FAILURE;
            }
        }
    }

    if(filename == NULL)
    {
        fprintf(stderr, "Error: Filename not specified.\n");
        return EXIT_FAILURE;
    }

    point_array_t *points = read_data(filename);
    if (points == NULL)
    {
        return EXIT_FAILURE;
    }

    point_array_t *clusters = init_clusters(points, k);
    if (clusters == NULL)
    {
        free(points->data);
        free(points);
        return EXIT_FAILURE;
    }

    run(points, clusters);
    write_results(points);

    free(points->data);
    free(points);
    free(clusters->data);
    free(clusters);
}
