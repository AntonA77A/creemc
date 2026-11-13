// Befehle (/help, /gamemode, /tp, ...).
#pragma once
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

class Player;
class Server;

class CommandSender {
public:
    virtual ~CommandSender() = default;
    virtual void sendMessage(const std::string& msg) = 0;
    virtual std::string senderName() const = 0;
    virtual bool senderIsOp() const = 0;
    virtual Player* asPlayer() { return nullptr; }
};

class ConsoleSender : public CommandSender {
public:
    void sendMessage(const std::string& msg) override;
    std::string senderName() const override { return "Konsole"; }
    bool senderIsOp() const override { return true; }
};

struct CommandError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

using CommandFunc = std::function<void(CommandSender&, const std::vector<std::string>&)>;
using CompleteFunc = std::function<std::vector<std::string>(CommandSender&, const std::vector<std::string>&)>;

struct Command {
    std::string name, description, usage;
    std::vector<std::string> aliases;
    bool opOnly = false;
    CommandFunc func;
    CompleteFunc completer;
};

class Commands {
public:
    explicit Commands(Server& server);
    void add(Command c);
    const Command* get(const std::string& name) const;
    std::vector<std::string> visibleNames(CommandSender& sender) const;
    bool dispatch(CommandSender& sender, const std::string& line);
    std::vector<std::string> complete(CommandSender& sender, const std::string& line);
    const std::map<std::string, Command>& all() const { return commands_; }

private:
    void registerBuiltins();
    Server& server_;
    std::map<std::string, Command> commands_;
    std::map<std::string, std::string> lookup_;   // Name/Alias -> Name
};
