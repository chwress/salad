Salad - A Content Anomaly Detector based on n-Grams
==

Letter Salad or Salad for short, is an efficient and flexible implementation of the well-known anomaly detection method Anagram by Wang et al.

It is based on n-gram models, that is, data is represented as all its substrings of length n. During training these n-grams are stored in a Bloom filter. This enables the detector to represent a large number of n-grams in little memory and still being able to efficiently access the data. Anagram was designed to operate on n-grams of bytes. Salad extends Anagram in this respect by generalizing this by also allowing n-grams of words or tokens. This is often referred to as bag-of-words and is a standard technique for mapping strings to vectors. Furthermore Salad implements a 2-class version of the detector such that next to anomaly detection also a classification of two classes can be performed. Analyses on the used data with respect to the n-gram model to be used can be performed with the build-in inspection mode and on basis of general statistics of the detector.

Salad can be utilized in various different fields of application. The concept underlying Salad was most prominently used for intrusion detection, but is not limited to this. Consult the manual page of Salad for more information.

Copyright (C) 2012-2013 Christian Wressnegger
