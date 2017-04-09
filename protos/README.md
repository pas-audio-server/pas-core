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
protoc -I=protos --cpp_out=protos --go_out=protos protos/{FILE}.proto
```

## To use