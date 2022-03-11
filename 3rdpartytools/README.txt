bin2coff: converts binary data file to linkable MS COFF object files

The generated object file can then be used with MS compilers (Visual Studio,
WDK) or MinGW (32 or 64 bit). The object file contains 2 variables, one being
the data itself and the other its size (32 bit unsigned value).

Current limitations:
- only little endian architectures are supported
- only x86 architectures are supported
- source must be 4 GB or less

These limitations can easily be overcome by modifying the source.
You must respect the GPL v3 (or later) license if you do so.

Usage: bin2coff bin obj [label] [64bit]

  bin  : source binary data
  obj  : target object file, in MS COFF format.
  label: identifier for the extern data. If not provided, the name of the
         binary file without extension is used.
  64bit: produce a 64 bit compatible object - symbols are generated without
         leading underscores and machine type is set to x86_x64.

With your linker set properly, typical access from a C source is:

    extern uint8_t  label[]     /* binary data         */
    extern uint32_t label_size  /* size of binary data */
