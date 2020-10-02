# memanalyzer
Code for sampling and analysis of computer program execution data (Linux)

For now it reads target process (PID) memory and tracks changes in memory use.

The final idea is to also collect kernel execution information and make computer
more "self-aware" by turning deep learning algorithms to analyse (also) internal
state which should then make a model where own internal state will be part of 
mathematical models that are used to understand world.

The preprocessing challenge is that data vectors are large (10^7 dimensions O(10 MB))
and only 3% of memory change in ~20 Hz sampling frequency. This means 
large part of data needs to be ignored. For this calculating PCA reduction
to keep 90% of variance without computing covariance matrix is needed.

Additionally, during program execution there are only small active spots
that keep changing so the area of memory change probably keeps changing 
rapidly and faster than our maximum speed sampling frequency 
(~ 20 times a second).

After reducting dimension of variables to something like 10^4 we can
then maybe compute cross correlation matrix Cyx when data representation
changes and compute linear transformation y = Ax to keep track of memory
representation changes during computation.
