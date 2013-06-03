// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
template <typename T>
T bitcount(T v) {
  static_assert((T)~0 > (T)0, "bitcount<T> only works for unsigned T");
  // Parallel computation of 2-bit bitcount into every 2 bits (3 ops)
  v = v - ((v >> 1) & (T)~(T)0/3);
  // Parallel summation into 4-bit bitcounts into every 4 bits (4 ops)
  v = (v & (T)~(T)0/15*3) + ((v >> 2) & (T)~(T)0/15*3);
  // Parallel summation into 8-bit bitcounts into of every 8 bits (3 ops)
  v = (v + (v >> 4)) & (T)~(T)0/255*15;
  // Summation of 8-bit bitcounts into top 8 bits and shift down (2 ops)
  return (T)(v * ((T)~(T)0/255)) >> (sizeof(T) - 1) * 8;
}
