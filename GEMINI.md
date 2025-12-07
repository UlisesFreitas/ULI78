# ULI78 Project Context and Guidelines

## 1. Project Overview
* **Name:** ULI78 Project
* **Purpose:** High-performance C++ utility/library focused on [Assume purpose, e.g., Data processing/System utilities].
* **Primary Language:** C++20.
* **Target Output:** The main application/binary is `Uli78.exe`.
* **Build System:** Managed externally by the user via `build.ps1`. **DO NOT** suggest build commands.

---

## 2. Technical Standards (Ready C++ Code)
The following rules are MANDATORY and must ensure C++ code is of the highest quality and immediately ready to compile:

* **C++ Standard:** **C++20**. Use modern features such as `std::ranges`, `std::unique_ptr` (RAII), and lambdas.
* **Performance:** Code generation must prioritize **optimal time complexity** (O(log N) or better) in data structures and algorithms.
* **Modularity:** **Strict separation** of interface (.hpp) and implementation (.cpp).
* **IO:** **Prohibited** is the use of C-style IO functions (`printf`, `scanf`). Use `std::cout` and `std::cin` exclusively.
* **Compilation:** **MANDATORY** correct inclusion of headers (`#include`) and use of the class scope resolution operator (`ClassName::method`).

---

## 3. Speed Optimization (Zero Latency Rules)
To ensure maximum speed and reduce latency, the following output restrictions apply:

* **Modification Method:** Always prioritize making changes **directly in the open file** (in-place edit).
* **Post-Edit Silence/Diff View Mandate:** Upon completing any requested in-place code modification, **ABSOLUTELY DO NOT** generate or send any descriptive text, confirmation summary, or completion message to the chat panel.
* **Strict Output Format:** **ONLY** provide code. **DO NOT** include introductions, explanations, or summaries in the chat, unless explicitly requested (e.g., using "Explain" or "Justify").
* **Command Prohibition:** **ABSOLUTELY DO NOT** generate or suggest any shell commands or build steps (`powershell`, `build.ps1`). The user handles execution externally.

---

## 4. Structure and Ignored Context
* **Context Priority:** Always prioritize context from the current file and its associated header file.
* **Noisy Directories (Ignore):** To accelerate analysis, **DO NOT** read or process the content of the following directories:
    * `uli78_pro/`
    * `build/`
    * `tests/`
    * `vendor/`
    * `vcpkg/`
    * `deps/`

---

## 5. Failure Diagnosis Protocol (RCA)
* **Failure Diagnosis Protocol:** If provided with any execution log, test failure report, or system error (especially concerning the 'Uli78.exe' binary), the assistant must immediately perform a **Root Cause Analysis (RCA)**.
* **RCA Output Format:** The response must be two-fold and follow the speed rules: 1) A single, concise sentence stating the **most probable cause**, and 2) The necessary **inline code modification** to fix the issue.