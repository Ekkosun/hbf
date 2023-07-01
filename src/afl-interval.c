#include <stdio.h>
#include <stdlib.h>

struct Interval {
  unsigned int start;
  unsigned int end;
  unsigned int frequency;
};

// Function to compare two intervals based on their start time.
int compareIntervals(const void *a, const void *b) {
  return ((struct Interval *)a)->start - ((struct Interval *)b)->start;
}

// Function to merge overlapping intervals.
struct Interval *mergeIntervals(struct Interval *intervals, int intervalsSize,
                                int *returnSize) {
  if (intervalsSize <= 0) {
    *returnSize = 0;
    return NULL;
  }

  // Sort the intervals based on their start time.
  qsort(intervals, intervalsSize, sizeof(struct Interval), compareIntervals);

  // Initialize the result array with the first interval.
  struct Interval *result =
      (struct Interval *)malloc(intervalsSize * sizeof(struct Interval));
  result[0].start = intervals[0].start;
  result[0].end = intervals[0].end;
  int size = 1;

  // Merge overlapping intervals.
  for (int i = 1; i < intervalsSize; i++) {
    if (intervals[i].start <= result[size - 1].end) {
      result[size - 1].end = intervals[i].end > result[size - 1].end
                                 ? intervals[i].end
                                 : result[size - 1].end;
    } else {
      result[size].start = intervals[i].start;
      result[size].end = intervals[i].end;
      size++;
    }
  }

  *returnSize = size;
  return result;
}

// Function to merge overlapping intervals.
struct Interval *deduplicateIntervals(struct Interval *intervals, int intervalsSize,
                                int *returnSize) {
  if (intervalsSize <= 0) {
    *returnSize = 0;
    return NULL;
  }

  // Sort the intervals based on their start time.
  qsort(intervals, intervalsSize, sizeof(struct Interval), compareIntervals);

  // Initialize the result array with the first interval.
  struct Interval *result =
      (struct Interval *)malloc(intervalsSize * sizeof(struct Interval));
  result[0].start = intervals[0].start;
  result[0].end = intervals[0].end;
  result[0].frequency = 0;
  int size = 1;

  // Merge overlapping intervals.
  for (int i = 1; i < intervalsSize; i++) {
    if ((intervals[i].start >= result[size - 1].start) && (intervals[i].end <= result[size - 1].end) ) {
        intervals[i].frequency ++ ;
    } else {
      result[size].start = intervals[i].start;
      result[size].end = intervals[i].end;
      size++;
    }
  }

  *returnSize = size;
  return result;
}
