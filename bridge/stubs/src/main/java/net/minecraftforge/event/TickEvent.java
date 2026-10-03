package net.minecraftforge.event;

/** Stub Forge tick event. */
public class TickEvent {
    public static class ServerTickEvent {
        public enum Phase { START, END }
        public final Phase phase;
        public ServerTickEvent(Phase phase) { this.phase = phase; }
    }
}
