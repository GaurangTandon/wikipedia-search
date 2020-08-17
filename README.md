# wikipedia-search
blazingly fast search for wikipedia, faster than google, deployed on aws

## Usage

1. Install libstudxml from [official source code](https://www.codesynthesis.com/projects/libstudxml/) (this project uses version 1.0.1)
2. At compile time include the file `libstudxml.a`, it's most likely present in `/usr/local/lib`. If not, you can check the output of the previous installation and locate the directory.
3. Compilation: `g++ parser.cpp /usr/local/lib/libstudxml.a -o parser`

Parser stats

On 150MB file: (commit 78b6889)

```
File read and parser readied in time 2.088e-05
Exhausted reading 4291 records in time 0.864192
```
