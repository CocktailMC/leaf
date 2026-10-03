dependencies {
    api(project(":common"))
    compileOnly(project(":stubs"))
}

tasks.jar {
    archiveBaseName.set("leaf-bootstrap-neoforge")
    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
    from(project(":common").tasks.named("jar").map { zipTree(it.outputs.files.singleFile) })
}
