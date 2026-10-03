package net.neoforged.neoforge.event;

public class ServerTickEvent {
    public enum Phase { START, END }
    public final Phase phase;
    public ServerTickEvent(Phase phase) { this.phase = phase; }
}
