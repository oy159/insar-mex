# InSAR Phase Unwrapping Toolkit

## Directory Structure

```
install/
├── bin/
│   ├── Radar_main.exe          # SNAPHU test program
│   └── snaphu.exe              # SNAPHU standalone CLI
├── lib/
│   ├── unwrapped_phase.lib     # Core phase unwrapping library
│   ├── snaphu.lib              # SNAPHU static library (in-memory API)
│   ├── mex/                    # MATLAB MEX plugins
│   │   ├── snaphu_mex.mexw64
│   │   ├── branchCut_mex.mexw64
│   │   ├── qualityMapGuideMethod_mex.mexw64
│   │   └── circularMedianFilter_mex.mexw64
│   └── cmake/insar_mex/        # CMake package config
├── include/insar_mex/          # Public headers
│   ├── utils/utils.h
│   ├── phaseUnwrap/snaphu.h
│   ├── phaseUnwrap/branchCut.h
│   ├── phaseUnwrap/qualityMapGuideMethod.h
│   └── filter/CircularMedianFilter.h
```

---

## Executables

### snaphu.exe

Original SNAPHU command-line tool. Reads/writes phase files from disk.

```bash
# Basic usage
snaphu.exe <input_file> <line_length> [options]

# Examples
snaphu.exe wrapped.phase 200 -o unwrapped.phase
snaphu.exe wrapped.phase 200 -o unwrapped.phase -m magnitude.mag -t
snaphu.exe wrapped.phase 200 -o unwrapped.phase --mcf -s
```

`line_length` = number of pixels per row (columns). Row count is inferred from file size.

| Option | Description |
|--------|-------------|
| `-o <file>` | Output unwrapped phase file |
| `-m <file>` | Magnitude file |
| `-c <file>` | Correlation file |
| `-a <file>` | Amplitude file (single file, ALT_SAMPLE format) |
| `--aa <f1> <f2>` | Amplitude from two separate files (one per pass) |
| `-t` | Topography mode (default) |
| `-d` | Deformation mode |
| `-s` | Smooth-solution mode |
| `--mst` | MST initialization (default) |
| `--mcf` | MCF initialization |
| `-h` | Full help |

#### Input File Formats

All files are **raw binary**, **32-bit IEEE 754 float**, **native byte order**. No header.

**COMPLEX_DATA** (default input format, `-f complex_data`)

Interleaved real/imaginary float pairs per pixel. Each pixel = 8 bytes.

```
[re(0,0) im(0,0)] [re(0,1) im(0,1)] ... [re(0,N-1) im(0,N-1)]   <- row 0
[re(1,0) im(1,0)] [re(1,1) im(1,1)] ... [re(1,N-1) im(1,N-1)]   <- row 1
...
```

- File size = `nrow * ncol * 2 * 4` bytes
- Magnitude = `sqrt(re^2 + im^2)`, Phase = `atan2(im, re)`

**ALT_LINE_DATA** (default output format, `-f alt_line`)

Alternating lines: magnitude row, then phase row, repeating. Total lines = `2 * nrow`.

```
[mag(0,0)  mag(0,1)  ... mag(0,N-1) ]   <- line 0 (magnitude row 0)
[phi(0,0)  phi(0,1)  ... phi(0,N-1) ]   <- line 1 (phase row 0)
[mag(1,0)  mag(1,1)  ... mag(1,N-1) ]   <- line 2 (magnitude row 1)
[phi(1,0)  phi(1,1)  ... phi(1,N-1) ]   <- line 3 (phase row 1)
...
```

- File size = `nrow * ncol * 2 * 4` bytes
- Used for: output (`-o`), magnitude (`-m`), correlation (`-c`)

**FLOAT_DATA** (`-f float_data`)

Raw float array, row-major. No magnitude channel.

```
[phi(0,0) phi(0,1) ... phi(0,N-1)]   <- row 0
[phi(1,0) phi(1,1) ... phi(1,N-1)]   <- row 1
...
```

- File size = `nrow * ncol * 4` bytes
- Used for: wrapped phase (when magnitude is provided separately via `-m`)

**ALT_SAMPLE_DATA** (`-f alt_sample_data`)

Alternating samples within each line.

```
[mag(0,0) phi(0,0) mag(0,1) phi(0,1) ... mag(0,N-1) phi(0,N-1)]   <- row 0
[mag(1,0) phi(1,0) mag(1,1) phi(1,1) ... mag(1,N-1) phi(1,N-1)]   <- row 1
...
```

- File size = `nrow * ncol * 2 * 4` bytes

#### Generating Input Files from MATLAB

```matlab
% wrappedPhase: M x N double matrix in [-pi, pi]
% magnitude:    M x N double matrix, non-negative

% Write as FLOAT_DATA (phase only)
fid = fopen('wrapped.phase', 'wb');
fwrite(fid, single(wrappedPhase), 'float32');   % column-major from MATLAB
fclose(fid);

% Write as ALT_LINE_DATA (magnitude + phase interleaved)
fid = fopen('wrapped.alt', 'wb');
for i = 1:size(wrappedPhase, 1)
    fwrite(fid, single(magnitude(i,:)), 'float32');     % magnitude line
    fwrite(fid, single(wrappedPhase(i,:)), 'float32');  % phase line
end
fclose(fid);

% snaphu usage:
%   snaphu.exe wrapped.alt 200 -o unwrapped.alt
%   snaphu.exe wrapped.phase 200 -o unwrapped.alt -m magnitude.mag
```

### Radar_main.exe

Test program that exercises SNAPHU unwrap with synthetic data. No arguments needed.

```bash
Radar_main.exe
```

---

## Static Libraries

### unwrapped_phase.lib

Core InSAR processing library. Provides phase unwrapping algorithms (branch cut, quality map guided) and utilities.

**CMake usage:**

```cmake
find_package(insar_mex REQUIRED)
target_link_libraries(my_app insar_mex::unwrapped_phase)
```

**C++ usage:**

```cpp
#include "phaseUnwrap/branchCut.h"
#include "utils/utils.h"

utils::MatrixD phase(rows, cols);
// ... fill phase data ...

branchCut::Method method;
auto result = method.unwrap(phase);
```

### snaphu.lib

SNAPHU library with in-memory API. No file I/O.

**CMake usage:**

```cmake
find_package(insar_mex REQUIRED)
target_link_libraries(my_app insar_mex::snaphu)
target_compile_definitions(my_app PRIVATE INSAR_MEX_SNAPHU_AVAILABLE=1)
```

**C++ usage:**

```cpp
#include "phaseUnwrap/snaphu.h"
#include "utils/utils.h"

utils::MatrixD wrappedPhase(M, N);
utils::MatrixD magnitude(M, N);
// ... fill data ...

// MST init, topography mode (default)
utils::MatrixD result = snaphu::unwrap(wrappedPhase);

// Full options
utils::MatrixD result = snaphu::unwrap(
    wrappedPhase,
    snaphu::MST,       // or snaphu::MCF
    snaphu::TOPO,       // or snaphu::DEFO, snaphu::SMOOTH
    &magnitude           // nullptr = all ones
);

if (result.rows() == 0) { /* failed */ }
```

---

## MATLAB MEX

Add `lib/mex` to MATLAB path before use:

```matlab
addpath('<install_dir>/lib/mex')
```

### snaphu_mex

SNAPHU phase unwrapping.

```matlab
unwrapped = snaphu_mex(wrappedPhase);
unwrapped = snaphu_mex(wrappedPhase, magnitude);
unwrapped = snaphu_mex(wrappedPhase, magnitude, 'mst', 'topo');
unwrapped = snaphu_mex(wrappedPhase, [], 'mcf', 'smooth');
```

| Parameter | Values | Default |
|-----------|--------|---------|
| wrappedPhase | double matrix (M x N) | required |
| magnitude | double matrix (M x N), or `[]` | all ones |
| initMethod | `'mst'`, `'mcf'` | `'mst'` |
| costMode | `'topo'`, `'defo'`, `'smooth'` | `'topo'` |

### branchCut_mex

Branch-cut phase unwrapping.

```matlab
[unwrapped, residues, branchCuts, numRes] = branchCut_mex(wrappedPhase);
```

### qualityMapGuideMethod_mex

Quality-map guided region-growing unwrapping.

```matlab
[unwrapped, mask, flag] = qualityMapGuideMethod_mex(qualityMap, phaseMap);
[unwrapped, mask, flag] = qualityMapGuideMethod_mex(qualityMap, phaseMap, threshold);
```

### circularMedianFilter_mex

Circular median filter for phase noise reduction.

```matlab
output = circularMedianFilter_mex(phaseMatrix, radius);
output = circularMedianFilter_mex(phaseMatrix, radius, verbose);
```

---

## CMake Integration

Other projects can link against the installed libraries:

```cmake
find_package(insar_mex REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app
    insar_mex::unwrapped_phase
    insar_mex::snaphu
)
target_compile_definitions(my_app PRIVATE INSAR_MEX_SNAPHU_AVAILABLE=1)
```
