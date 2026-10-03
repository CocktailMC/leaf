dependencies {
    api(project(":common"))

    val useReal = (findProperty("leaf.useRealLoaderApis") as String?) == "true"
    if (useReal) {
        // Real Fabric Loader for ModInitializer; Fabric API events still stubbed
        // until a Loom Minecraft environment is wired.
        compileOnly("net.fabricmc:fabric-loader:0.16.14")
        compileOnly(project(":stubs"))
    } else {
        compileOnly(project(":stubs"))
    }
}

tasks.jar {
    archiveBaseName.set("leaf-bootstrap-fabric")
    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
    from(project(":common").tasks.named("jar").map { zipTree(it.outputs.files.singleFile) })
}
