dependencies {
    api(project(":common"))

    val useReal = (findProperty("leaf.useRealLoaderApis") as String?) == "true"
    if (useReal) {
        // Official Forge maven artifacts vary heavily by MC version; keep stubs
        // as the default and document Loom/FG for production game runs.
        compileOnly(project(":stubs"))
    } else {
        compileOnly(project(":stubs"))
    }
}

tasks.jar {
    archiveBaseName.set("leaf-bootstrap-forge")
    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
    from(project(":common").tasks.named("jar").map { zipTree(it.outputs.files.singleFile) })
}
