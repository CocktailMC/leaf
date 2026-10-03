# Hex Error Codes (`0xSSMMEEEE`)

## Format

```text
0xSSMMEEEE
SS   = Severity   (00 success, 01 info, 02 error, 03 fatal, 04 panic)
MM   = Module ID  (permanent allocation)
EEEE = Error No.  (0x0000 reserved)
```

Example: `0x02030006` = ERROR / EVENT / RECURSION_LIMIT.

## ABI

- C: `abi/include/leaf/abi/leaf_error_v1.h` (`leaf_error_code`, `LEAF_OK`, macros)
- C++: `leaf::error_code` (`uint32_t`), `leaf::ec::*`, `leaf::error` (code + optional message)

**Only the numeric code is a long-term protocol.** Messages/symbols may improve.

## Lookup

```cpp
leaf::format_error_code(code); // "0x02030006"
leaf::error_symbol(code);      // "LEAF_EVENT_RECURSION_LIMIT"
leaf::error_message(code);     // human text
leaf::module_name(code);       // "event"
```

Log style:

```text
[LEAF][ERROR][0x02030006] LEAF_EVENT_RECURSION_LIMIT: ...
```
