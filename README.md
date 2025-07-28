# mysh — A Full-Featured Linux Shell in C

🧠 `mysh` is a custom Linux shell written in C, built to replicate and extend the behavior of standard shells like `bash`. It includes support for built-in commands, process management, piping, background tasks, and even client-server communication — all built from scratch as part of a systems programming course.

---

## ⚙️ Features

### ✅ Core Shell Functionality
- `echo`, `cd`, `ls`, `cat`, `wc`, `exit`
- Custom prompt (`mysh$`) and error handling
- Token parsing and whitespace normalization
- Variable expansion (e.g., `$var`) with unlimited storage

### 📂 File System Support
- Recursive directory listing with `ls`
- Advanced `cd` support with shorthand paths like `...` → `../../`
- File reading, display, and word counting

### 🔀 Pipes and Background Tasks
- UNIX-style piping (`|`) for built-in and external commands
- Background execution with `&`, displaying `[n] pid` info
- `ps` to list active processes
- `kill [pid] [signal]` to terminate specific tasks
- Proper signal handling with graceful termination messages (`Done`)

### 🌐 Networking
- Built-in non-blocking chat server with:
  - `start-server <port>`
  - `start-client <port> <host>`
  - `send <port> <host> <msg>`
  - `close-server`
- Each client gets a unique ID (e.g., `client#1`) and can send/receive real-time messages
- `\connected` to view the number of connected clients

---

## 🧪 How to Compile & Run

### 🔨 Compilation
To compile:
```bash
make
```

To clean object files and the binary:
```bash
make clean
```

Makefile uses:
```bash
-g -Wall -Wextra -Werror -fsanitize=address,leak,object-size,bounds-strict,undefined -fsanitize-address-use-after-scope
```

### ▶️ Running
After compiling:
```bash
./mysh
```

You will see the prompt:
```
mysh$ 
```

---

## 🕹️ Usage Examples

```bash
mysh$ echo hello world
hello world

mysh$ MYVAR=hi
mysh$ echo $MYVAR
hi

mysh$ ls ./ --rec --d 2
# Recursively lists directory up to 2 levels deep

mysh$ cat file.txt | wc
# Pipes file content to wc

mysh$ sleep 10 &
[1] 8472

mysh$ ps
sleep 8472

mysh$ kill 8472
```

### Networking
```bash
mysh$ start-server 5000
mysh$ start-client 5000 localhost
client#1> Hello!
client#1> \connected
```

---

## 🛠 Design Highlights

- Clean modular code with separation between I/O, parsing, and execution
- Extensive use of `fork()`, `exec()`, `pipe()`, and signal handling
- Manual memory management with heap-based variable storage
- Non-blocking sockets and `select()` used in client/server implementation
- Failsafe on invalid inputs (e.g., `ERROR: Unknown command`)

---

## 🎓 Context

This project was developed as part of **CSC209: Software Tools and Systems Programming** at the University of Toronto. Each milestone progressively added complexity — from simple echo handling to building a fully functioning concurrent shell with networking.

All components, including process management, piping, and networking, were implemented individually without third-party libraries or `system()` calls.

---

## 🙌 Author

**Dipal Hingorani**  
Builder at heart. Systems enthusiast. Proud of clean abstractions and precise execution.  
*All code written from scratch.*
