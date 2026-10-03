package leafmc.sdk

import leaf.abi.*
import kotlinx.cinterop.*

/** Read-only view of a contiguous Leaf event packet. */
@OptIn(ExperimentalForeignApi::class)
class EventView(private val ptr: COpaquePointer?) {
    val isValid: Boolean get() = ptr != null

    fun eventId(): UInt {
        if (ptr == null) return 0u
        return ptr.reinterpret<LeafEventHeaderV1>().pointed.event_id
    }
}

@OptIn(ExperimentalForeignApi::class)
fun asPlayerJoin(view: EventView): ULong {
    // Payload follows LeafEventHeaderV1 in the contiguous packet.
    // Real decoding needs proper pointer arithmetic against leaf_event_v1.h.
    return 0u
}
