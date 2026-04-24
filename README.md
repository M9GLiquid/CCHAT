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

Use the VS Code tasks in this repository for the common build flows.

```bash
meson setup Build -Dbuild_server=true -Dbuild_client=false
meson compile -C Build
```

Build the server and client:

```bash
meson setup Build -Dbuild_server=true -Dbuild_client=true
meson compile -C Build
```

Build the client only:

```bash
meson setup Build -Dbuild_server=false -Dbuild_client=true
meson compile -C Build
```

In VS Code, the matching tasks are:

- `Meson: Build Server`
- `Meson: Build Server & Client`
- `Meson: Build Client`
- `Meson: Compile`
- `Meson: Wipe`

## Launch

Start the server:

```bash
./Build/cchat_server
```

Start the server on a custom port:

```bash
./Build/cchat_server 5555
```

If the client target is enabled:

```bash
./Build/cchat_client
```

## Documentations
[CZMQ API Documentation](https://github.com/zeromq/czmq#using-czmq)  
[ZMQ API Documentation](https://libzmq.readthedocs.io/en/latest/)  
