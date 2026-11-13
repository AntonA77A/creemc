package org.bukkit.configuration.file;

import java.util.List;

public class FileConfigurationOptions {
    private final FileConfiguration config;
    boolean copyDefaults;
    private String header;

    protected FileConfigurationOptions(FileConfiguration config) { this.config = config; }

    public FileConfiguration configuration() { return config; }

    public boolean copyDefaults() { return copyDefaults; }

    public FileConfigurationOptions copyDefaults(boolean value) {
        copyDefaults = value;
        return this;
    }

    public String header() { return header; }

    public FileConfigurationOptions header(String value) {
        header = value;
        return this;
    }

    public FileConfigurationOptions setHeader(List<String> value) {
        header = value == null ? null : String.join("\n", value);
        return this;
    }

    public FileConfigurationOptions copyHeader(boolean value) { return this; }

    public FileConfigurationOptions parseComments(boolean value) { return this; }
}
