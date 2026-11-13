package org.bukkit.configuration.file;

import java.io.File;
import java.io.IOException;
import java.io.Reader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import org.bukkit.configuration.Configuration;
import org.bukkit.configuration.InvalidConfigurationException;
import org.bukkit.configuration.MemoryConfiguration;

public abstract class FileConfiguration extends MemoryConfiguration {
    private FileConfigurationOptions options;

    public FileConfiguration() { }

    public FileConfiguration(Configuration defaults) { super(defaults); }

    public FileConfigurationOptions options() {
        if (options == null) options = new FileConfigurationOptions(this);
        return options;
    }

    public abstract String saveToString();

    public abstract void loadFromString(String contents) throws InvalidConfigurationException;

    public void save(File file) throws IOException {
        File parent = file.getAbsoluteFile().getParentFile();
        if (parent != null) parent.mkdirs();
        Files.writeString(file.toPath(), saveToString(), StandardCharsets.UTF_8);
    }

    public void save(String file) throws IOException { save(new File(file)); }

    public void load(File file) throws IOException, InvalidConfigurationException {
        loadFromString(Files.readString(file.toPath(), StandardCharsets.UTF_8));
    }

    public void load(String file) throws IOException, InvalidConfigurationException { load(new File(file)); }

    public void load(Reader reader) throws IOException, InvalidConfigurationException {
        StringBuilder b = new StringBuilder();
        char[] buf = new char[4096];
        int n;
        while ((n = reader.read(buf)) > 0) b.append(buf, 0, n);
        loadFromString(b.toString());
    }
}
