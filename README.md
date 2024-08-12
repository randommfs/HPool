<h1 align="center">
  <br>
  <a href="https://github.com/randommfs/HPool/archive/master.zip">HPool</a>
</h1>

<h4 align="center">Simple and blazingly fast pool allocator implementation</h4>

<p align="center">
    <a href="https://github.com/randommfs/HPool/issues">
    <img src="https://img.shields.io/github/issues/randommfs/HPool?color=lime"
         alt="GitHub opened issues">
    <img src="https://img.shields.io/badge/status-stable-lime"
         alt="Status">
    <img src="https://img.shields.io/github/license/randommfs/HPool?color=lime"
         alt="License">
    <img src="https://img.shields.io/github/stars/randommfs/HPool?color=lime"
         alt="Stars">
</p>

---
<table>
<tr>
<td>
Why this thing is so fast? The only place where O(n) method is used is in next free block search function. When you call allocate(), it takes index of free block, and searches for next free block starting from that index. It works, because free() always keeps special variable (which contains index of free block) pointing to the *first* free block. Also, free() uses O(1) algorithm, so every run it takes constant time. Thats main features of library - insanely fast free algorithm and small header!
</td>
</tr>
</table>

# Table of contents
- [Installation](#installation)
- [Building tests and benchmarks](#building-tests-and-benchmarks)
- [Usage](#usage)
- [Benchmarks](#benchmarks)

## Installation
1. Download main header [hpool.h](hpool.hpp)
2. Include it to your source file.
3. ...use?

## Building tests and benchmarks
There is single test (yes) and some benchmarks. To build, you can do this:
```bash
mkdir build
cd build
cmake ..
make
./tests # Run test(s)
./bench # Run benchmark (please, be patient)
```

## Usage

### Preparation
Create object like that:
```cpp
hpool::HPool<T> pool{32};
```
where T is a target type, and 32 is a maximum pool size (in elements).

### Use
#### Allocate memory:
```cpp
auto ptr = pool.allocate();
```

#### Free memory:
```cpp
pool.free(ptr);
```

#### Get total count of elements:
```cpp
uint32_t count = pool.get_total_elements();
```

#### Get count of allocated elements:
```cpp
uint32_t count = pool.get_allocated_elements();
```

## Benchmarks

I compared this library to boost's object_pool using my own benchmarking lib called [HBench](https://github.com/randommfs/HBench). You can find it in [bench](bench) directory in project root.

**Pools has been tested in different scopes, on 524288 elements. Compiled with -O3. CPU - AMD FX-8350 4GHz. Test results are average from 3 iterations.**

| Benchmark                       	| Result      	|
|---------------------------------	|-------------	|
| boost::object_pool  allocation  	| 5ms        	|
| boost::object_pool  linear free 	| 5m 27s 404ms |
| boost::object_pool  random free 	| 2m 2s 500ms 	|
| hpool::HPool  allocation        	| 5ms         	|
| hpool::HPool  linear free       	| 1ms         	|
| hpool::HPool  random free       	| 5ms         	|