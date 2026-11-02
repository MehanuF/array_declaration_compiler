#!/usr/bin/env python3
"""
Array Declaration Compiler — Web Interface
A professional front-end workbench for C-like array declaration analysis.
Phases: Lexical Analysis -> Syntax Analysis (AST) -> Semantic Verification
"""

import os
import re
import socket
import subprocess
import tempfile
from pathlib import Path
import pandas as pd
import streamlit as st

# ── Paths ──────────────────────────────────────────────────────
ROOT = Path(__file__).resolve().parent
BINARY = ROOT / "array_compiler"
TESTS_DIR = ROOT / "tests" / "inputs"
GRAMMAR_FILE = ROOT / "grammar" / "array_grammar.txt"

# ── Sample Programs ────────────────────────────────────────────
DEFAULT_EXAMPLE = """\
int marks[3] = {10, 20, 30};
float values[5];
char grade[1];
marks[1 + 1] = 50;
"""

PRESET_EXAMPLES = {
    "Default Multi-Statement Program": DEFAULT_EXAMPLE,
    "Valid: Basic Array Declaration (v1.c)": "int arr[5];\n",
    "Valid: Float Array Declaration (v2.c)": "float values[10];\n",
    "Valid: Char Array Declaration (v3.c)": "char letters[4];\n",
    "Valid: Array with Initializer List (v4.c)": "int marks[3] = {10, 20, 30};\n",
    "Valid: Array Element Assignment (v6.c)": "int arr[5];\narr[2] = 50;\n",
    "Valid: Arithmetic Expression in Index (v8.c)": "int arr[10];\narr[1 + 2] = 8;\n",
    "Valid: Expressions in Initializer (v9.c)": "int a[4] = {1 + 2, 5, 8, 10};\n",
    "Syntax Error: Missing Identifier (i1.c)": "int [5];\n",
    "Syntax Error: Missing Semicolon (i2.c)": "int arr[5]\n",
    "Syntax Error: Missing Size Dimension (i3.c)": "int arr[];\n",
    "Syntax Error: Unbalanced Bracket (i4.c)": "int arr[5 = 2;\n",
    "Syntax Error: Unclosed Initializer (i5.c)": "int arr[3] = {1, 2, 3;\n",
    "Syntax Error: Incomplete Assignment (i6.c)": "int arr[5];\narr[2] = ;\n",
    "Syntax Error: Leading Comma in Initializer (i7.c)": "int arr[3] = {, 1};\n",
    "Semantic Error: Undeclared Array Access (s1.c)": "unknown[1] = 5;\n",
    "Semantic Error: Literal Index Out of Bounds (s2.c)": "int arr[5];\narr[5] = 10;\n",
    "Semantic Error: Initializer Exceeds Declared Size (s3.c)": "int arr[3] = {1, 2, 3, 4};\n",
    "Semantic Error: Non-Positive Array Dimension (s4.c)": "int arr[0];\n",
}

# ── Network Detection ──────────────────────────────────────────
def get_network_ip():
    """Detect LAN IP address for internal network distribution."""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.settimeout(0.2)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"

# ── Compiler Execution ─────────────────────────────────────────
def ensure_binary():
    """Verify presence of compiler binary; compile using make if necessary."""
    if BINARY.exists():
        return True, "Compiler binary ready."
    try:
        res = subprocess.run(["make"], cwd=str(ROOT), capture_output=True, text=True)
        if res.returncode == 0 and BINARY.exists():
            return True, "Compiler binary built successfully via Makefile."
        return False, f"Build failure:\n{res.stdout}\n{res.stderr}"
    except Exception as e:
        return False, f"Makefile invocation failed: {e}"

def compile_source(source_text: str):
    """Execute compiler binary against supplied source code."""
    ok, msg = ensure_binary()
    if not ok:
        return -1, "", msg

    tmp_file = None
    try:
        with tempfile.NamedTemporaryFile("w", suffix=".c", delete=False, encoding="utf-8") as f:
            f.write(source_text + "\n")
            tmp_file = f.name

        proc = subprocess.run([str(BINARY), tmp_file], capture_output=True, text=True)
        return proc.returncode, proc.stdout, proc.stderr
    except Exception as e:
        return -1, "", f"Execution error: {e}"
    finally:
        if tmp_file and os.path.exists(tmp_file):
            try:
                os.unlink(tmp_file)
            except OSError:
                pass

# ── Parsers ────────────────────────────────────────────────────
def parse_token_stream(stdout: str):
    """Parse lexical tokens from Phase 1 output."""
    in_section = False
    tokens = []
    token_re = re.compile(r'^\s{2}(\S+)\s+(\S+)\s+line\s+(\d+),\s+col\s+(\d+)')

    for line in stdout.splitlines():
        if "PHASE 1" in line:
            in_section = True
            continue
        if "PHASE 2" in line:
            break
        if in_section:
            m = token_re.match(line)
            if m:
                ttype, lexeme, ln, col = m.groups()
                tokens.append({
                    "Index": len(tokens) + 1,
                    "Token Type": ttype,
                    "Lexeme": lexeme,
                    "Line": int(ln),
                    "Column": int(col)
                })
    return tokens

def parse_ast_lines(stdout: str):
    """Parse AST hierarchy from Phase 2 output."""
    in_section = False
    ast_lines = []
    for line in stdout.splitlines():
        if "PHASE 2" in line:
            in_section = True
            continue
        if "PHASE 3" in line or line.startswith("Result:"):
            break
        if in_section:
            if line.strip() and not line.startswith("┌") and not line.startswith("│") and not line.startswith("└"):
                ast_lines.append(line)
    return ast_lines

def generate_ast_graphviz(ast_lines):
    """Generate structured Graphviz DOT syntax representing the parse tree."""
    if not ast_lines:
        return None

    nodes = []
    edges = []
    stack = []

    for i, line in enumerate(ast_lines):
        if "|--" in line:
            idx = line.index("|--")
            depth = 1 + (idx - 2) // 6
            content = line[idx + 4:].strip()
        else:
            depth = 0
            content = line.strip()

        node_id = f"node_{i}"
        
        fillcolor = "#1e2030"
        fontcolor = "#cad3f5"
        bordercolor = "#5b6078"

        if "[PROGRAM]" in content:
            fillcolor = "#24273a"
            fontcolor = "#8aadf4"
            bordercolor = "#8aadf4"
        elif "[DECL]" in content:
            fillcolor = "#182c25"
            fontcolor = "#a6da95"
            bordercolor = "#a6da95"
        elif "[ASSIGN]" in content:
            fillcolor = "#352718"
            fontcolor = "#f5a97f"
            bordercolor = "#f5a97f"
        elif "[INIT_LIST]" in content:
            fillcolor = "#2c203b"
            fontcolor = "#c6a0f6"
            bordercolor = "#c6a0f6"
        elif "[ARRAY_ACCESS]" in content:
            fillcolor = "#182b35"
            fontcolor = "#7dc4e4"
            bordercolor = "#7dc4e4"
        elif "[BINOP]" in content:
            fillcolor = "#361d2b"
            fontcolor = "#f5bde6"
            bordercolor = "#f5bde6"
        elif "[INT_LIT]" in content:
            fillcolor = "#202a22"
            fontcolor = "#a6da95"
            bordercolor = "#8bd5ca"

        safe_label = content.replace('"', '\\"')
        nodes.append((node_id, safe_label, fillcolor, fontcolor, bordercolor))

        while stack and stack[-1][0] >= depth:
            stack.pop()

        if stack:
            parent_id = stack[-1][1]
            edges.append((parent_id, node_id))

        stack.append((depth, node_id))

    dot = [
        "digraph AST {",
        "  rankdir=TB;",
        "  bgcolor=\"transparent\";",
        "  node [shape=box, style=\"rounded,filled\", fontname=\"monospace\", fontsize=10, penwidth=1.2];",
        "  edge [color=\"#8aadf4\", penwidth=1.0, arrowhead=vee];"
    ]
    for nid, lbl, fill, fontc, borderc in nodes:
        dot.append(f'  {nid} [label="{lbl}", fillcolor="{fill}", fontcolor="{fontc}", color="{borderc}"];')
    for p, c in edges:
        dot.append(f"  {p} -> {c};")
    dot.append("}")
    return "\n".join(dot)

def parse_semantics(stdout: str, stderr: str, rc: int):
    """Extract symbol table entries and semantic check feedback."""
    in_section = False
    rows = []
    for line in stdout.splitlines():
        if "PHASE 3" in line:
            in_section = True
            continue
        if in_section:
            rows.append(line)

    sym_header_seen = False
    symbols = []
    status_line = ""

    for row in rows:
        stripped = row.strip()
        if "Symbol Table" in stripped:
            sym_header_seen = True
            continue
        if sym_header_seen and (stripped.startswith("Name") or stripped.startswith("----")):
            continue
        if sym_header_seen and stripped and not stripped.startswith("Semantic") and not stripped.startswith("Result"):
            parts = stripped.split()
            if len(parts) >= 4:
                symbols.append({
                    "Identifier": parts[0],
                    "Type": parts[1],
                    "Size": parts[2],
                    "Declared Line": " ".join(parts[3:])
                })
        if "Semantic analysis" in stripped:
            status_line = stripped
            sym_header_seen = False

    err_lines = [l for l in (stderr or "").splitlines() if l.strip()]
    return symbols, status_line, err_lines

# ── Application Configuration ──────────────────────────────────
st.set_page_config(
    page_title="Array Declaration Compiler — Workbench",
    layout="wide",
    initial_sidebar_state="expanded"
)

# Professional Typography and Component Styles
st.markdown("""
<style>
    body, [data-testid="stAppViewContainer"] {
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    }
    .main-title {
        font-size: 24px;
        font-weight: 700;
        letter-spacing: -0.02em;
        color: #f4f4f6;
        margin-bottom: 2px;
    }
    .sub-title {
        color: #9399b2;
        font-size: 13px;
        margin-bottom: 20px;
    }
    .badge-pass {
        display: inline-block;
        background-color: #104e35;
        color: #a6da95;
        border: 1px solid #1e6b4a;
        padding: 4px 10px;
        border-radius: 4px;
        font-weight: 600;
        font-size: 12px;
        letter-spacing: 0.04em;
    }
    .badge-fail {
        display: inline-block;
        background-color: #4c1d24;
        color: #ed8796;
        border: 1px solid #6b2832;
        padding: 4px 10px;
        border-radius: 4px;
        font-weight: 600;
        font-size: 12px;
        letter-spacing: 0.04em;
    }
    div.stButton > button {
        border-radius: 4px;
        font-weight: 600;
        letter-spacing: 0.01em;
    }
</style>
""", unsafe_allow_html=True)

# ── State Initialization ───────────────────────────────────────
if "editor_code" not in st.session_state:
    st.session_state["editor_code"] = DEFAULT_EXAMPLE

# ── Sidebar ────────────────────────────────────────────────────
with st.sidebar:
    st.markdown("### Array Declaration Compiler")
    st.caption("Simplified C Front End (Lexical, Syntax, Semantics)")
    st.divider()

    st.markdown("#### Test Presets")
    selected_preset = st.selectbox(
        "Select benchmark or sample program:",
        options=list(PRESET_EXAMPLES.keys()),
        index=0
    )

    if st.button("Load Program", use_container_width=True):
        st.session_state["editor_code"] = PRESET_EXAMPLES[selected_preset]
        st.rerun()

    st.divider()

    st.markdown("#### Source File Upload")
    uploaded_file = st.file_uploader("Upload C source file (.c)", type=["c", "txt"])
    if uploaded_file is not None:
        try:
            content = uploaded_file.read().decode("utf-8")
            if st.button("Apply Uploaded Source", use_container_width=True):
                st.session_state["editor_code"] = content
                st.rerun()
        except Exception as e:
            st.error(f"Failed to read file: {e}")

    st.divider()

    st.markdown("#### Compiler Engine")
    if BINARY.exists():
        st.markdown('<span class="badge-pass">BINARY ACTIVE</span>', unsafe_allow_html=True)
    else:
        st.markdown('<span class="badge-fail">BINARY MISSING</span>', unsafe_allow_html=True)
        if st.button("Build Binary (make)", use_container_width=True):
            ok, msg = ensure_binary()
            if ok:
                st.success(msg)
                st.rerun()
            else:
                st.error(msg)

    st.divider()

    st.markdown("#### Network Addresses")
    lan_ip = get_network_ip()
    st.text(f"Local:   http://localhost:8501\nNetwork: http://{lan_ip}:8501")

    with st.expander("Public Access Configuration"):
        st.markdown(f"""
        To generate a public tunnel endpoint:
        
        **Method 1 (localtunnel):**
        ```bash
        npx localtunnel --port 8501
        ```
        
        **Method 2 (SSH Reverse Tunnel):**
        ```bash
        ssh -p 443 -R0:localhost:8501 a.pinggy.io
        ```
        """)

# ── Main Content Area ──────────────────────────────────────────
st.markdown('<div class="main-title">Array Declaration Compiler — Analysis Workbench</div>', unsafe_allow_html=True)
st.markdown('<div class="sub-title">Phase 1: Lexical Analyzer &nbsp;|&nbsp; Phase 2: Recursive Descent Parser & AST &nbsp;|&nbsp; Phase 3: Semantic Verification</div>', unsafe_allow_html=True)

col_editor, col_output = st.columns([1, 1.25], gap="large")

with col_editor:
    st.markdown("##### Source Code")
    code_input = st.text_area(
        label="Source Code Input",
        value=st.session_state["editor_code"],
        height=330,
        label_visibility="collapsed"
    )
    st.session_state["editor_code"] = code_input

    btn_col1, btn_col2, btn_col3 = st.columns([1.4, 1, 1])
    with btn_col1:
        st.button("Compile & Analyze", use_container_width=True, type="primary")
    with btn_col2:
        if st.button("Clear Editor", use_container_width=True):
            st.session_state["editor_code"] = ""
            st.rerun()
    with btn_col3:
        if st.button("Reset Example", use_container_width=True):
            st.session_state["editor_code"] = DEFAULT_EXAMPLE
            st.rerun()

    lines_count = len(code_input.splitlines()) if code_input else 0
    chars_count = len(code_input) if code_input else 0
    st.caption(f"Lines: {lines_count} | Characters: {chars_count}")

    st.download_button(
        label="Download Source (.c)",
        data=code_input,
        file_name="program.c",
        mime="text/x-c",
        use_container_width=True
    )

# ── Compilation Execution ──────────────────────────────────────
source_to_run = code_input.strip()
if source_to_run:
    rc, stdout, stderr = compile_source(source_to_run)
else:
    rc, stdout, stderr = None, "", ""

with col_output:
    st.markdown("##### Analysis Results")

    if not source_to_run:
        st.info("Provide source code in the editor and click 'Compile & Analyze' to run.")
    elif rc == -1:
        st.error(f"Binary execution error: {stderr}")
    else:
        # Verdict Header
        if rc == 0:
            st.markdown('<div class="badge-pass" style="font-size:14px; padding:6px 14px; margin-bottom:12px;">ACCEPTED — All Phases Validated</div>', unsafe_allow_html=True)
        elif rc == 1:
            st.markdown('<div class="badge-fail" style="font-size:14px; padding:6px 14px; margin-bottom:12px;">REJECTED — Syntax Error Detected (Phase 2)</div>', unsafe_allow_html=True)
        elif rc == 2:
            st.markdown('<div class="badge-fail" style="font-size:14px; padding:6px 14px; margin-bottom:12px;">REJECTED — Semantic Error Detected (Phase 3)</div>', unsafe_allow_html=True)
        else:
            st.markdown(f'<div class="badge-fail" style="font-size:14px; padding:6px 14px; margin-bottom:12px;">REJECTED — Exit Code {rc}</div>', unsafe_allow_html=True)

        tab_tokens, tab_ast, tab_sem, tab_raw, tab_tests, tab_grammar = st.tabs([
            "Phase 1: Tokens",
            "Phase 2: AST",
            "Phase 3: Semantics",
            "Raw Terminal",
            "Test Suite",
            "Grammar Specification"
        ])

        # ── Tab 1: Tokens ──
        with tab_tokens:
            st.markdown("###### Lexical Analysis (Token Stream)")
            tokens = parse_token_stream(stdout)
            if tokens:
                col_m1, col_m2, col_m3 = st.columns(3)
                with col_m1:
                    st.metric("Total Tokens", len(tokens))
                with col_m2:
                    unique_types = len(set(t["Token Type"] for t in tokens))
                    st.metric("Unique Types", unique_types)
                with col_m3:
                    max_line = max(t["Line"] for t in tokens)
                    st.metric("Scanned Lines", max_line)

                df_tokens = pd.DataFrame(tokens)
                st.dataframe(
                    df_tokens,
                    column_config={
                        "Index": st.column_config.NumberColumn("#", width="small"),
                        "Token Type": st.column_config.TextColumn("Token Type", width="medium"),
                        "Lexeme": st.column_config.TextColumn("Lexeme", width="medium"),
                        "Line": st.column_config.NumberColumn("Line", width="small"),
                        "Column": st.column_config.NumberColumn("Column", width="small"),
                    },
                    hide_index=True,
                    use_container_width=True,
                    height=280
                )
            else:
                st.warning("No tokens generated from the lexical scanner.")

        # ── Tab 2: AST ──
        with tab_ast:
            st.markdown("###### Syntax Analysis & Abstract Syntax Tree")
            if rc == 1:
                st.error("Syntax Error encountered. Parse tree generation aborted.")
                err_lines = [l for l in (stderr or stdout).splitlines() if "Syntax Error" in l or "syntax error" in l or "expected" in l]
                if err_lines:
                    for err in err_lines:
                        st.code(err, language="text")
            else:
                ast_lines = parse_ast_lines(stdout)
                if ast_lines:
                    view_mode = st.radio("Tree Visualization Format:", ["Graphviz Diagram", "Text Hierarchy"], horizontal=True)
                    if view_mode == "Graphviz Diagram":
                        dot_src = generate_ast_graphviz(ast_lines)
                        if dot_src:
                            st.graphviz_chart(dot_src, use_container_width=True)
                        else:
                            st.info("Unable to generate Graphviz representation.")
                    else:
                        st.code("\n".join(ast_lines), language="text")
                else:
                    st.info("No parse tree entries found in output.")

        # ── Tab 3: Semantics ──
        with tab_sem:
            st.markdown("###### Semantic Verification & Symbol Table")
            symbols, status_line, sem_errs = parse_semantics(stdout, stderr, rc)

            st.markdown("**Symbol Table**")
            if symbols:
                df_sym = pd.DataFrame(symbols)
                st.dataframe(df_sym, hide_index=True, use_container_width=True)
            else:
                st.caption("No array symbols declared in current scope.")

            if rc == 2 and sem_errs:
                st.markdown("**Semantic Violations**")
                for err in sem_errs:
                    st.error(err)

            st.markdown("**Semantic Rule Enforcement**")
            rules = [
                ("Rule 1: Array declared before usage", "undeclared" not in stderr.lower()),
                ("Rule 2: Index expression resolves to integer", "integer type" not in stderr.lower()),
                ("Rule 3: Literal index within declared array bounds", "out of bounds" not in stderr.lower()),
                ("Rule 4: Type compatibility on initialization and assignment", "type mismatch" not in stderr.lower() and "incompatible" not in stderr.lower()),
                ("Rule 5: Initializer count within declared capacity", "exceeds" not in stderr.lower() and "too many" not in stderr.lower()),
                ("Rule 6: Positive non-zero array dimension", "non-positive" not in stderr.lower() and "size 0" not in stderr.lower()),
            ]

            checklist_data = []
            for name, passed in rules:
                status = "PASS" if (rc == 0 or passed) else "VIOLATED"
                checklist_data.append({"Rule": name, "Status": status})
            
            df_check = pd.DataFrame(checklist_data)
            st.dataframe(
                df_check,
                column_config={
                    "Rule": st.column_config.TextColumn("Specification Rule"),
                    "Status": st.column_config.TextColumn("Status", width="small")
                },
                hide_index=True,
                use_container_width=True
            )

        # ── Tab 4: Raw Terminal ──
        with tab_raw:
            st.markdown("###### Compiler Process Output")
            full_output = stdout
            if stderr:
                full_output += "\n--- STDERR ---\n" + stderr
            st.code(full_output, language="text")

        # ── Tab 5: Test Suite ──
        with tab_tests:
            st.markdown("###### Automated Regression Test Suite")
            st.caption("Executes all 21 test cases in tests/inputs/ and validates exit codes.")

            if st.button("Execute Full Test Suite", use_container_width=True):
                test_files = sorted(list(TESTS_DIR.glob("*.c")))
                results = []
                bar = st.progress(0)

                for idx, tf in enumerate(test_files):
                    name = tf.name
                    if name.startswith("v"):
                        expected_rc = 0
                        category = "Valid Program"
                    elif name.startswith("i"):
                        expected_rc = 1
                        category = "Syntax Error"
                    elif name.startswith("s"):
                        expected_rc = 2
                        category = "Semantic Error"
                    else:
                        expected_rc = 0
                        category = "General"

                    code_content = tf.read_text(encoding="utf-8")
                    act_rc, _, _ = compile_source(code_content)
                    passed = (act_rc == expected_rc)

                    results.append({
                        "Test Case": name,
                        "Category": category,
                        "Expected Exit": expected_rc,
                        "Actual Exit": act_rc,
                        "Verdict": "PASS" if passed else "FAIL"
                    })
                    bar.progress((idx + 1) / len(test_files))

                df_res = pd.DataFrame(results)
                passed_count = sum(1 for r in results if r["Verdict"] == "PASS")
                total_count = len(results)

                if passed_count == total_count:
                    st.success(f"Results: {passed_count}/{total_count} tests passed (100% compliance).")
                else:
                    st.warning(f"Results: {passed_count}/{total_count} tests passed.")

                st.dataframe(df_res, use_container_width=True, hide_index=True)

        # ── Tab 6: Grammar ──
        with tab_grammar:
            st.markdown("###### Formal Context-Free Grammar (CFG)")
            if GRAMMAR_FILE.exists():
                st.code(GRAMMAR_FILE.read_text(encoding="utf-8"), language="text")
            else:
                st.info("Grammar definition file not found in repository.")

st.divider()
st.caption("Array Declaration Compiler — Computer Science Project Reference Implementation")
