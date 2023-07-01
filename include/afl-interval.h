#ifndef MERGE_INTERVALS_H
#define MERGE_INTERVALS_H

/*
 * the file for  intervals merge
 */
struct Interval {
  unsigned int start;
  unsigned int end;
  unsigned int frequency;
};

struct Interval *mergeIntervals(struct Interval *intervals, int intervalsSize,
                                int *returnSize);

struct Interval *deduplicateIntervals(struct Interval *intervals, int intervalsSize,
                                int *returnSize);
#endif
