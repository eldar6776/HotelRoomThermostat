# AGENTS.md

# GIT SAFETY & RESTORE POINT POLICY (MANDATORY)

## PRIMARY RULE

Before any modification of code, configuration, scripts, documentation, specs, task files, or project structure, a local restore point MUST exist.
Before editing any file, read it first. Before modifying a function, grep for all callers. Research before you edit.
No edits are allowed before this procedure is completed.

---

## REQUIRED EXECUTION ORDER

## STEP 0 — PRE-CODE DESIGN GRILL (MANDATORY)

Before implementing any non-trivial change, the agent must establish explicit shared understanding with the user.

The agent must:

- identify the exact goal
- identify affected files/modules
- identify dependencies between decisions
- expose assumptions
- convert assumptions into direct questions
- identify risks
- mark blockers
- produce a concise implementation plan
- wait for approval before editing

The agent must not start coding while blockers, ambiguity, or hidden assumptions exist.

If request is unclear:

STOP AND ASK.

### DOMAIN-AWARENESS EXTENSION

Before proposing changes, review available project context first.

Examples:

- README.md
- STATUS.md
- TASKS.md
- SPECS.md
- CONTEXT.md
- docs/adr/
- existing naming conventions
- existing architecture notes

Use existing domain language consistently.

Do not invent new terminology if established terminology already exists.

If business terms are ambiguous, ask for clarification before coding.

---

## STEP 0.5 — ARCHITECTURE & REFACTOR DISCIPLINE

For architecture review, refactoring, scaling, module redesign, technical debt, or performance work, the agent must analyze the current structure before proposing changes.

The agent must identify:

- shallow modules with low value
- oversized modules with too many responsibilities
- duplicated logic across files
- hidden coupling between modules
- weak interfaces
- missing boundaries
- testing pain points
- areas where complexity leaks to callers
- unstable dependencies
- circular dependencies
- god classes / god services / god viewmodels
- repeated database access patterns
- tightly coupled startup/bootstrap logic

Use these principles:

- prefer deep modules with simple interfaces
- concentrate complexity internally
- reduce change spread across many files
- preserve working behavior
- prefer incremental refactors over rewrites
- if deletion of a module changes nothing, question its value
- reduce public surface area when possible
- favor explicit boundaries over implicit coupling

For refactors, the agent must provide:

1. current problem
2. proposed target shape
3. smallest safe migration path
4. risks
5. rollback path
6. validation plan

No large refactor may begin without approval.

---

## STEP 0.75 — ZOOM-OUT MODE

When working inside unfamiliar code, unclear modules, legacy areas, or complex systems, the agent must zoom out before editing.

The agent must identify:

- what this module exists to do
- who calls it
- what it depends on
- what side effects it has
- what boundaries it crosses
- whether it owns logic or only forwards calls
- what breaks if it changes
- nearby related modules

Do not patch local symptoms before understanding surrounding flow.

---

### STEP 1 — VERIFY REPOSITORY STATE

Run repository status check first.

Examples:

- git status --short
- git status

Determine whether the repository is:

- CLEAN = no pending changes
- DIRTY = existing local changes present

Do not skip this step.

---

### STEP 2 — CREATE RESTORE POINT

## If repository is DIRTY

Create a local restore point BEFORE any edits using one of these approved methods:

### Preferred Method A — Local Git Checkpoint Commit

Create a local commit containing current state.

Commit message format:

checkpoint: before codex edit YYYY-MM-DDTHH-MM-SS

### Approved Method B — Named Git Stash

Use only when commit is not appropriate.

Stash label format:

checkpoint-before-codex-YYYY-MM-DDTHH-MM-SS

---

## If repository is CLEAN

Create restore point only if planned work is medium/high risk:

Examples:

- multi-file refactor
- delete / rename operations
- architecture changes
- dependency changes
- generated code overwrite
- automated batch edits
- schema changes
- build pipeline changes
- startup/bootstrap changes

For trivial one-line safe edits, restore point may be skipped.

---

## STEP 3 — VERIFY CHECKPOINT SUCCESS

Confirm the restore point was created successfully.

If commit / stash failed:

- STOP immediately
- explain exact reason
- request approval if elevated permissions are required
- do not continue editing

No exceptions.

---

## STEP 4 — ONLY THEN START WORK

After restore point exists:

1. Read authoritative project instructions
2. Read relevant specs/tasks/status files
3. Make changes
4. Validate result

---

## STEP 5 — DIAGNOSE MODE (BUGS / FAILURES / PERFORMANCE)

For bugs, regressions, crashes, unstable behavior, wrong output, or slow performance, the agent must use a disciplined diagnosis loop.

Required order:

1. reproduce issue
2. minimize reproduction case
3. create repeatable feedback loop
4. rank plausible hypotheses
5. instrument targeted signals
6. apply smallest fix
7. regression test
8. remove temporary debug noise

Do not guess root cause without evidence.

Do not apply speculative fixes when issue has not been reproduced.

For performance issues:

- measure before changing
- compare before/after
- identify bottleneck first

---

## STEP 6 — IMPLEMENTATION MODE (TDD / SMALL SAFE STEPS)

For features or fixes, prefer small vertical slices.

Recommended loop:

1. define expected behavior
2. create failing test when practical
3. implement minimal change
4. verify pass
5. refactor safely
6. repeat

Rules:

- keep project buildable after each step
- avoid giant patches
- prefer many small correct changes over one large risky change
- preserve existing behavior unless intentionally changing it

If tests are unavailable, state that explicitly.

---

## STRICT FAILURE RULES

The agent MUST stop work if:

- git permission denied
- index.lock issue
- merge conflict blocks checkpoint
- repository unavailable
- git command failed
- uncertain repository state
- requested change is ambiguous
- required files cannot be found
- system state cannot be verified
- destructive action lacks approval

Do not improvise.

Do not continue without checkpoint.

---

## CHANGE TRANSPARENCY RULE

Before edits, clearly state:

- repository state (clean / dirty)
- chosen restore method (commit / stash)
- checkpoint identifier created
- intended scope of edits
- validation plan

After edits, clearly state:

- files changed
- validation performed
- known limitations
- how to rollback if needed

---

## ROLLBACK AWARENESS

Always preserve ability to return user to previous state quickly.

Preferred rollback methods:

- git reset --hard <checkpoint_commit>
- git stash pop
- git switch previous branch

(Use only when user requests.)

---

## PROHIBITED BEHAVIOR

Never:

- start editing before repository check
- hide failed checkpoint creation
- assume rollback exists
- overwrite many files without restore point
- continue after git errors
- silently discard user changes
- refactor unrelated areas without approval
- rename/move/delete broadly without approval
- guess technical facts as certainty
- claim validation was done when it was not

---

## NON-NEGOTIABLE DECISION PRIORITY

When speed conflicts with safety:

SAFETY WINS.

When uncertainty conflicts with progress:

STOP AND ASK.

When edits conflict with restore point policy:

RESTORE POINT FIRST.

When local fix conflicts with system design:

ZOOM OUT FIRST.

When debugging conflicts with guessing:

MEASURE FIRST.

---

## DEFAULT OPERATING MODE

Assume all meaningful changes require rollback capability.

If unsure whether restore point is needed:

Create one.

If unsure whether design is clear:

Ask first.

If unsure whether architecture is affected:

Analyze first.

If unsure whether bug cause is known:

Reproduce first.

