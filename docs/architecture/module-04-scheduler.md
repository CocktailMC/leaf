# Module 4 — Scheduler

## Responsibilities

```text
Event Bus  = what happened
Scheduler  = when / which thread runs work
```

Provides:

- `post_main` — Minecraft main thread queue (+ optional inline)
- `post_async` — worker pool
- `post_main` / `delay_main` / `cancel_task` (LeafApiV1.7)
- `pump_main` — Bridge/tests drain the main queue
- `bind_main_thread` — establish thread ownership

## Thread rules

- `main_sync` events may `publish` only on the bound main thread.
- Off-main publishers must `scheduler.post_main([&]{ events.publish(...); })`.
- No silent thread hopping inside Event Bus.

## C ABI

`LeafApiV1::{post_main,delay_main,cancel_task}` forward to the engine scheduler.
`delay_main` is one-shot main-thread work; units are milliseconds. Tasks become
runnable after `pump_main` (each server tick from live bridges).

## Integration

`mod_engine` owns `scheduler` + `event_runtime`, attaches them, and binds main
at first `pump_main` from the Minecraft Bridge.
