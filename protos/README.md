# pas protos

Here are some useful [Protocol Buffer](https://developers.google.com/protocol-buffers/) definitions.

## To edit

### Installation

1. Go to [this Release page](https://github.com/google/protobuf/releases/tag/v3.0.0) for the download.
2. Download the proper `protoc-3.0.0-{OS/ARCH}.zip` file.
3. Follow the readme instructions from the download (i.e. put the `protoc` binary in your `PATH`).
4. Download the Golang helper: `go get -u github.com/golang/protobuf/protoc-gen-go`.

### Edit the file

Make edits as you normally would.

### Updating the generated code

Just run this command (assuming you are in the root of `pas`):

```bash
protoc -I=protos --cpp_out=protos/cpp --go_out=protos/go protos/{FILE}.proto
```

## To use

### C++

Just include the `pas` namespace:

```cpp
#include "path/to/protos/cpp/{FILE}.pb.h"
```

### Go

Just import the `pas` proto package:

```golang
import pas "path/to/protos/go"
```