# CChat

Tiny C chat project built with Meson and Ninja.

You can find the Roadmap [here!](docs/ROADMAP.md)

## Requirements

- `gcc`
- `meson`
- `ninja`
- `pkg-config`
- `libgtk-4-dev`
- `libczmq-dev`

On WSL with Ubuntu 24.04, you can install everything with:

```bash
chmod +x scripts/setup_ubuntu.sh
./scripts/setup_ubuntu.sh
```

If you already have `Ubuntu-24.04` in WSL for other projects, reuse that distro for CChat rather than creating a separate environment.

## Build

Build the server:

```bash
meson setup build
meson compile -C build
```

Build server and client:

```bash
meson setup build -Dbuild_client=true
meson compile -C build
```

## Launch

Start the server:

```bash
./build/cchat_server
```

Start the server on a custom port:

```bash
./build/cchat_server 5555
```

If the client target is enabled:

```bash
./build/cchat_client
```

## Documentations
[CZMQ API Documentation](https://github.com/zeromq/czmq#using-czmq)  
[ZMQ API Documentation](https://libzmq.readthedocs.io/en/latest/)  