Salad - A Content Anomaly Detector based on n-Grams
==

Letter Salad or Salad for short, is an efficient and flexible implementation of
the well-known anomaly detection method Anagram by Wang et al. (RAID 2006)

Salad enables detecting anomalies in large-scale string data. The tool is based
on the concepts of n-grams, that is, strings are compared using all substrings
of length n. During training, these n-grams are extracted from a collection of
strings and stored in a Bloom filter. This enables the detector to represent a
large number of n-grams in very little memory. During anomaly detection, the
n-grams of unknown strings are matched against the Bloom filter and strings
containing several n-grams not seen during training are flagged as anomalous.

Salad extends the original method Anagram in different ways: First, the tool
does not only operate on n-grams of bytes, but is also capable of comparing
n-grams over words and tokens. Second, Salad implements a 2-class version of the
detector that enables discriminating strings of two types. Finally, the tool
features a build-in inspection and statistic mode that can help to analyze the
learned Bloom filter and its predictions.

The tool can be utilized in different fields of application. For example, the
concept underlying Salad has been prominently used for intrusion detection, but
is not limited to this scenario. To illustrate the versatility of Salad we
provide some concrete examples of its usage. All examples come with data sets
and instructions.

Copyright (C) 2012-2013 Christian Wressnegger
