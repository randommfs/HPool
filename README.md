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
HPool is a lightweight pool allocator supporting multiple types and dynamic extension.
</td>
</tr>
</table>

# Table of contents
- [Installation](#installation)
- [Building tests](#building-tests-and-benchmarks)
- [Usage](#usage)
- [Benchmarks](#benchmarks)

## Installation
The recommended way (for now) is using git submodules. If you want to integrate HPool into your project, add it as a submodule.

## Building tests
To build tests, you need GTest available globally.
```bash
cmake -S. -Bbuild 
cmake --build build --target tests
build/tests
```

## Usage

### Preparation
Create pool object.
If you want a single-type pool:
```c++
// Create a pool, that holds 32 ints.
HPool::HPool<int> pool(32);
```
If you want a multi-type pool:
```c++
// Create a combined pool, that can hold 32 objects of either int or string.
HPool::HPool<int, std::string> pool(32);
```

### Use
#### Allocate memory:
```cpp
auto ptr = pool.allocate();
```
For single-type pool, `ptr` has type `HPool::Ptr<int>`.  
For multi-type pool, `ptr` has type `HPool::Ptr<int, std::variant<int, std::string>>`.

#### Free memory:
```cpp
pool.free(ptr);
```

#### Get total count of elements:
```cpp
uint32_t count = pool.size();
```

#### Get count of allocated elements:
```cpp
uint32_t count = pool.allocated();
```
