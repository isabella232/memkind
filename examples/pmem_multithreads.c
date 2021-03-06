/*
 * Copyright (c) 2018 - 2019 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <memkind.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define PMEM_MAX_SIZE (1024 * 1024 * 32)
#define NUM_THREADS 10

static char path[PATH_MAX]="/tmp/";

void *thread_ind(void *arg);

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static void print_err_message(int err)
{
    char error_message[MEMKIND_ERROR_MESSAGE_SIZE];
    memkind_error_message(err, error_message, MEMKIND_ERROR_MESSAGE_SIZE);
    fprintf(stderr, "%s\n", error_message);
}

int main(int argc, char *argv[])
{
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [pmem_kind_dir_path]\n", argv[0]);
        return 1;
    } else if (argc == 2 && (realpath(argv[1], path) == NULL)) {
        fprintf(stderr, "Incorrect pmem_kind_dir_path %s\n", argv[1]);
        return 1;
    }

    fprintf(stdout,
            "This example shows how to use multithreading with independent pmem kinds."
            "\nPMEM kind directory: %s\n", path);

    pthread_t pmem_threads[NUM_THREADS];
    int t;

    // Create many independent threads
    for (t = 0; t < NUM_THREADS; t++) {
        if (pthread_create(&pmem_threads[t], NULL, thread_ind, NULL) != 0) {
            fprintf(stderr, "Unable to create a thread.\n");
            return 1;
        }
    }

    sleep(1);
    if (pthread_cond_broadcast(&cond) != 0) {
        fprintf(stderr, "Unable to broadcast a condition.\n");
        return 1;
    }

    for (t = 0; t < NUM_THREADS; t++) {
        if (pthread_join(pmem_threads[t], NULL) != 0) {
            fprintf(stderr, "Thread join failed.\n");
            return 1;
        }
    }

    fprintf(stdout, "Threads successfully allocated memory in the PMEM kinds.\n");

    return 0;
}

void *thread_ind(void *arg)
{
    struct memkind *pmem_kind;

    if (pthread_mutex_lock(&mutex) != 0) {
        fprintf(stderr, "Failed to acquire mutex.\n");
        return NULL;
    }
    if (pthread_cond_wait(&cond, &mutex) != 0) {
        fprintf(stderr, "Failed to block mutex on condition.\n");
        return NULL;
    }
    if (pthread_mutex_unlock(&mutex) != 0) {
        fprintf(stderr, "Failed to release mutex.\n");
        return NULL;
    }

    int err = memkind_create_pmem(path, PMEM_MAX_SIZE, &pmem_kind);
    if (err) {
        print_err_message(err);
        return NULL;
    }

    void *test = memkind_malloc(pmem_kind, 32);
    if (test == NULL) {
        fprintf(stderr, "Unable to allocate pmem (test) in thread.\n");
        return NULL;
    }

    memkind_free(pmem_kind, test);

    err = memkind_destroy_kind(pmem_kind);
    if (err) {
        print_err_message(err);
    }

    return NULL;
}
