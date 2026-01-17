# miniRDBMS

Simple RDBMS in C with Web CRUD Demo

Overview

This project implements a simple relational database management system (RDBMS) in C, built from first principles. It supports a SQL-like interface, persistent storage, basic indexing, constraints, and query execution.

To demonstrate real-world usability, the RDBMS is integrated with a minimal HTTP web application that performs full CRUD (Create, Read, Update, Delete) operations through SQL.

The focus of the project is correctness, clarity, and systems-level understanding rather than performance or feature completeness.

⸻

Features

Database Engine
	•	SQL-like language
	•	Interactive parsing and execution pipeline
	•	Persistent storage (file-based)
	•	Table catalog and schema validation
	•	Supported statements:
	•	CREATE TABLE
	•	INSERT
	•	SELECT
	•	UPDATE
	•	DELETE
	•	Constraints:
	•	PRIMARY KEY (hash-indexed)
	•	UNIQUE
	•	Basic WHERE clause support
	•	INNER JOIN (nested-loop implementation)

Indexing
	•	Hash index for primary keys
	•	Index maintenance on INSERT / DELETE / UPDATE
	•	Used for constraint enforcement and fast lookups

Web Demonstration
	•	Minimal HTTP server written in C
	•	Exposes CRUD operations over HTTP
	•	SQL is the only interface to the database
	•	No external frameworks or libraries

⸻

Project Structure

.
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.c              # Entry point
│   ├── lexer.c / lexer.h   # SQL lexer
│   ├── parser.c / parser.h # SQL parser + AST
│   ├── ast.h               # AST definitions
│   ├── executor.c          # Statement execution
│   ├── storage.c           # Storage layer
│   ├── index.c             # Hash index for primary keys
│   ├── catalog.c           # Table metadata
│   └── web.c               # HTTP server (CRUD demo)
└── data/
    ├── table.tbl           # Table storage files


⸻

Build Instructions

Requirements
	•	GCC or Clang
	•	CMake
	•	POSIX-compatible system (Linux / macOS)

Build

mkdir build
cd build
cmake ..
make

This produces a single executable.

⸻

Running the Project

./rdbms [repl [,http]]

On startup, a REPL mode or the HTTP server is launched:

HTTP server listening on http://localhost:8080

The data/ directory will be created automatically to store tables and metadata.

⸻

Web CRUD Demonstration

The web app manages a simple users table.

Create Table

curl -X POST http://localhost:8080/create \
-d 'CREATE TABLE users (
    id INT PRIMARY KEY,
    name TEXT,
    email TEXT UNIQUE
);'


⸻

Insert Rows

curl -X POST http://localhost:8080/insert \
-d 'INSERT INTO users VALUES (1, "Alice", "alice@example.com");'

curl -X POST http://localhost:8080/insert \
-d 'INSERT INTO users VALUES (2, "Bob", "bob@example.com");'


⸻

Select (READ)

curl http://localhost:8080/select \
-d 'SELECT * FROM users;'

Output (server stdout):

1 Alice alice@example.com
2 Bob bob@example.com


⸻

Update

curl -X POST http://localhost:8080/update \
-d 'UPDATE users SET name = "Alicia" WHERE id = 1;'


⸻

Delete

curl -X POST http://localhost:8080/delete \
-d 'DELETE FROM users WHERE id = 2;'


⸻

Select (after update and delete)

curl http://localhost:8080/select \
-d 'SELECT * FROM users;'

1 Alicia alice@example.com


⸻

INNER JOIN Example

Tables

CREATE TABLE users (
    id INT PRIMARY KEY,
    name TEXT
);

CREATE TABLE orders (
    id INT PRIMARY KEY,
    user_id INT
);

Query

SELECT *
FROM users
INNER JOIN orders
ON users.id = orders.user_id;

Execution strategy:
	•	Schema validation
	•	Nested-loop join
	•	Value-based equality match

⸻

Design Notes
	•	The web server does not access storage directly.
	•	All database interaction goes through SQL.
	•	The engine is deterministic and single-threaded.
	•	Storage format is intentionally simple and inspectable.
	•	Indexing is explicit and manually maintained.

⸻

Limitations (By Design)
	•	No transactions
	•	No concurrency
	•	No query optimizer
	•	No cost-based planning
	•	Limited SQL grammar

These trade-offs keep the implementation understandable and auditable.

⸻

Educational Outcomes

This project demonstrates:
	•	Lexer and parser construction
	•	AST-based execution
	•	Persistent storage design
	•	Index implementation
	•	Constraint enforcement
	•	End-to-end system integration
	•	Separation of concerns between database and application

⸻
