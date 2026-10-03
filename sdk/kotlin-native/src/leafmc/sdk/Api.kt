package leafmc.sdk

import leaf.abi.*
import kotlinx.cinterop.*

/**
 * Thin Kotlin/Native façade over [LeafApiV1].
 * Mirrors `sdk/cpp/include/leaf/sdk/api.hpp`.
 */
@OptIn(ExperimentalForeignApi::class)
class Api(private val raw: CPointer<LeafApiV1>?) {
    val isValid: Boolean get() = raw != null

    fun log(level: Int, message: String) {
        val api = raw?.pointed ?: return
        val fn = api.log ?: return
        message.encodeToByteArray().usePinned { pinned ->
            fn(level, pinned.addressOf(0))
        }
    }

    fun info(message: String) = log(LEAF_LOG_INFO.toInt(), message)
    fun warn(message: String) = log(LEAF_LOG_WARN.toInt(), message)

    fun hasCapability(cap: UInt): Boolean {
        val api = raw?.pointed ?: return false
        val fn = api.has_capability ?: return false
        return fn(cap) != 0
    }

    fun getServer(): ULong {
        val api = raw?.pointed ?: return 0u
        val fn = api.get_server ?: return 0u
        return fn()
    }

    fun sendPlayerMessage(player: ULong, message: String): LeafStatus {
        val api = raw?.pointed ?: return LeafStatus.LEAF_STATUS_NOT_SUPPORTED
        val fn = api.send_player_message ?: return LeafStatus.LEAF_STATUS_NOT_SUPPORTED
        return message.encodeToByteArray().usePinned { pinned ->
            fn(player, pinned.addressOf(0))
        }
    }

    fun broadcastMessage(message: String): LeafStatus {
        val api = raw?.pointed ?: return LeafStatus.LEAF_STATUS_NOT_SUPPORTED
        val fn = api.broadcast_message ?: return LeafStatus.LEAF_STATUS_NOT_SUPPORTED
        return message.encodeToByteArray().usePinned { pinned ->
            fn(pinned.addressOf(0))
        }
    }

    fun playerCount(): UInt {
        val api = raw?.pointed ?: return 0u
        val fn = api.player_count ?: return 0u
        return fn()
    }
}
