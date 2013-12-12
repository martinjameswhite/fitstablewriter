fitstablewriter
===============

Simple interface to create FITS binary tables.

This code was designed to be a very lightweight, header-only
library to generate simple FITS binary tables from C++.  So
far only a serial version exists, and only for some data types.
Extension to an MPI-parallel version and more data types will
follow if the need arises.

This code currently supports only the situation where a number of
arrays of equal length need to be written to a binary table on HDU 1.
This is the situation I encounter most frequently when writing FITS
binary tables.
