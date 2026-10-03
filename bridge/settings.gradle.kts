pluginManagement {
    repositories {
        mavenCentral()
        gradlePluginPortal()
    }
}

rootProject.name = "leafmc-bridges"

include("stubs")
include("common")
include("fabric")
include("forge")
include("neoforge")
include("harness")
