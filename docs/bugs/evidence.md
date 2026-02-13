ChakraSilicon/docs/bugs/evidence.md
# ARM64 JIT Bugs: Evidence Gathering

## Investigation Prompt

Your task is to conduct a thorough, fact-based investigation into the ARM64 JIT bugs documented in the following files:

- ARM64_JIT_ARGUMENT_CORRUPTION.md
- ARM64_JIT_CALLDIRECT_VARARGS.md
- JIT_CALL_BUGS_STATUS.md

**Instructions:**

1. **Gather Evidence:**  
   - Review the above documentation and any related code.
   - Test using the JIT-enabled compiler to observe behaviors and collect concrete evidence.

2. **Document Known Facts:**  
   - Record only what you can directly observe or verify.
   - Maintain and continually update this file with your findings.

3. **Do Not Speculate:**  
   - Avoid theorizing or proposing solutions.
   - Do not attempt to fix any issues at this stage.

4. **Understand Code Aspects:**  
   - Map out and document relevant code stacks and execution flows.
   - Describe how different parts of the code interact, especially around the JIT and ARM64-specific logic.

5. **Identify Use Cases & Scenarios:**  
   - Enumerate all distinct use cases or scenarios affected by these bugs.
   - For each, document the observed behavior and any relevant stack traces or flows.

6. **Stay Focused:**  
   - This is a deep fact-finding expedition. The problem is difficult and there is no quick solution.
   - Do not get distracted by unrelated issues or by attempts to resolve the bugs.

**Goal:**  
Build a comprehensive, factual record of the problem, its manifestations, and the code paths involved. This will serve as the foundation for future debugging and resolution efforts.

---

## Scenario Template

### Scenario: [Title/Description]

#### Facts:
- [List all directly observed facts relevant to this scenario.]

#### Evidence:
- [Concrete evidence: logs, test results, stack traces, code references.]

#### Diagram:
- [Insert diagram or flowchart illustrating the code path, stack, or interaction.]

#### Stack/Flow Documentation:
- [Describe the relevant stack trace or code execution flow.]

#### Use Cases:
- [Enumerate affected use cases or variations.]

#### Additional Notes:
- [Any clarifying information, strictly factual.]

---

*Update this file as you gather new evidence. Each scenario should be documented separately using the above template.*