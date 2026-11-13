package net.kyori.adventure.text.logger.slf4j;

import net.kyori.adventure.text.Component;

public interface ComponentLogger extends org.slf4j.Logger {
    void info(Component msg);

    void warn(Component msg);

    void error(Component msg);
}
