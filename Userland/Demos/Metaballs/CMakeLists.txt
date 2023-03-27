serenity_component(
    Metaballs
    TARGETS Metaballs
)

set(SOURCES
    Metaballs.cpp
)

serenity_app(Metaballs ICON app-Metaball)
target_link_libraries(Metaballs PRIVATE LibDesktop LibGUI LibCore LibGfx LibMain)
