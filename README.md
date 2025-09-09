# Qt Chat Server (C++ / Qt)

Real-time chat server and admin UI built with C++ and Qt. Handles multi-client connections over TCP, JSON-based messaging, client presence/status, typing indicators, and message routing.

## Highlights
- **C++ / Qt** server built on `QTcpServer` / `QTcpSocket`
- **JSON protocol** for `init`, `login`, `message`, `is_typing`, `set_name`, `set_status`
- **Client management**: unique IDs, names, statuses; broadcast client list
- **Routing**: direct messages, server broadcast, echo-back for sender UI sync
- **Admin UI**: tabs per client, live status, typing indicator

## Components
- `ServerManager` — TCP server, message dispatch, routing, presence
- `DroneChatApp` — GUI that shows connected clients/tabs and live status
- `MainWindow` — entry/login gate to admin UI
- `ChatItemWidget` — styled message bubble with timestamps (used in UI)

## Quick Start
1. Open the `.pro` in **Qt Creator** (Qt Widgets).
2. Build and run. The server listens on **port 4500** by default.
3. Connect a client that speaks the JSON line protocol (newline-delimited).

## Protocol (quick reference)
- `{"type":"init","os":"<str>","version":"<str>"}` → `{"type":"init_ack","status":"ok"}`
- `{"type":"login","username":"test","password":"test"}` → `{"type":"login_ack","status":"ok"|"error","reason":""}`
- `{"type":"set_name","name":"Alice"}`
- `{"type":"set_status","status":1}`  *(1=Available, 2=Away, 3=Busy, 4=Offline)*
- `{"type":"message","recipient":<id>,"content":"Hello"}`  → forwarded to recipient and echoed to sender
- Server broadcasts: `{"type":"client_list","clients":[{id,name,status}, ...]}`

> Note: On connect, the server assigns an `id` via `{"type":"assign_id","id":<int>}`.


## Acknowledgment
This was a collaborative project. I focused on the server-side implementation and the admin UI integration described above.

## License
MIT
