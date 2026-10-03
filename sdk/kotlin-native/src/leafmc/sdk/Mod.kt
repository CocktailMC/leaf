package leafmc.sdk

import leaf.abi.*
import kotlinx.cinterop.*

/**
 * Base class for Kotlin/Native Leaf Mods.
 * Export `leaf_mod_entry` from a KN dynamic library the same way C++ does.
 */
@OptIn(ExperimentalForeignApi::class)
abstract class Mod {
    data class Info(val id: String, val name: String, val version: String)

    abstract fun info(): Info

    open fun onLoad(api: Api) {}
    open fun onEnable(api: Api) {}
    open fun onDisable(api: Api) {}
    open fun onUnload(api: Api) {}
}

/**
 * Helper to fill [LeafModInfoV1] / [LeafModExportsV1] from a [Mod] instance.
 * Call from your exported `leaf_mod_entry`.
 */
@OptIn(ExperimentalForeignApi::class)
fun fillModEntry(
    mod: Mod,
    api: CPointer<LeafApiV1>?,
    outInfo: CPointer<LeafModInfoV1>?,
    outExports: CPointer<LeafModExportsV1>?,
): LeafStatus {
    val info = mod.info()
    outInfo?.pointed?.apply {
        struct_size = sizeOf<LeafModInfoV1>().toUInt()
        // Stable C strings must outlive the call — store in process statics in real mods.
        id = info.id.cstr.getPointer(Arena())
        name = info.name.cstr.getPointer(Arena())
        version = info.version.cstr.getPointer(Arena())
    }
    // Export binding is toolchain-specific; see README for leaf_mod_entry pattern.
    outExports?.pointed?.struct_size = sizeOf<LeafModExportsV1>().toUInt()
    return LeafStatus.LEAF_STATUS_OK
}
