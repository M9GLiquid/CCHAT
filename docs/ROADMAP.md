## TODO

Status: `▢` Open, `◔` Ongoing, `✓` Done

### Dependencies

- ✓ Add CZMQ as the messaging backend dependency
- ✓ Change platform to Linux (Ubuntu 24.04)

### Architecture

- ▢ Replace pthread-based ZMQ networking with CZMQ zactors
- ▢ Restructure the project into dedicated modules instead of grouping everything into `server`, `client`, and `state`

### Client UI

- ▢ Replace the terminal client with a proper windowed client UI
- ▢ Show the client id in the terminal prompt so the user can see their assigned id and where to type input

### Rooms

- ▢ Add rooms to connect to

### Commands

- ▢ Restrict commands to utility actions like creating channels and listing commands
- ▢ Add a text parser utility class for commands
- ▢ Add a command to list connected users

### Authentication

- ▢ Add login support

### Data

- ▢ Add database support

### Access Control

- ▢ Add permissions for channels and users

### Security

- ▢ Improve security across client-server communication
