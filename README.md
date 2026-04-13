# CChat

Tiny C chat project built with Meson and Ninja.

## Requirements

- `gcc`
- `meson`
- `ninja`

On Ubuntu/Debian you can install everything with:

```bash
chmod +x scripts/setup_ubuntu.sh
./scripts/setup_ubuntu.sh
```

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
