#!/usr/bin/env python3
"""
Array Declaration Compiler — GUI
Runs the compiler binary and displays output in colour-coded panels.

Usage:
    python3 gui.py
"""

import os
import re
import subprocess
import sys
import tempfile
import threading
from pathlib import Path

import tkinter as tk
from tkinter import filedialog, font, messagebox, ttk

# ── paths ──────────────────────────────────────────────────────
ROOT   = Path(__file__).resolve().parent
BINARY = ROOT / "array_compiler"

# ── colour palette ─────────────────────────────────────────────
BG        = "#1e1e2e"   # dark background
PANEL     = "#2a2a3e"   # panel background
BORDER    = "#3c3c5a"   # panel border / separator
FG        = "#cdd6f4"   # default text
FG_DIM    = "#6c7086"   # dimmed / labels
ACCENT    = "#89b4fa"   # blue accent (phase headings)
GREEN     = "#a6e3a1"   # accepted / ok
RED       = "#f38ba8"   # error
YELLOW    = "#f9e2af"   # warning / literal
CYAN      = "#89dceb"   # token type
PURPLE    = "#cba6f7"   # AST nodes
PINK      = "#f5c2e7"   # numbers / values
EDITOR_BG = "#181825"   # code editor bg
BTN_BG    = "#89b4fa"   # compile button bg
BTN_FG    = "#1e1e2e"   # compile button text

FONT_CODE = ("Consolas", 12)
FONT_UI   = ("Segoe UI", 10)
FONT_HEAD = ("Segoe UI", 10, "bold")
FONT_BIG  = ("Segoe UI", 16, "bold")


EXAMPLE_CODE = """\
int marks[3] = {10, 20, 30};
float values[5];
char grade[1];
marks[1 + 1] = 50;
"""


# ══════════════════════════════════════════════════════════════
class CompilerGUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Array Declaration Compiler")
        self.geometry("1300x800")
        self.minsize(1000, 600)
        self.configure(bg=BG)

        self._compile_running = False
        self._build_ui()
        self._load_example()

    # ── UI construction ────────────────────────────────────────
    def _build_ui(self):
        self._build_header()
        self._build_body()
        self._build_status_bar()

    def _build_header(self):
        bar = tk.Frame(self, bg=BG, pady=12, padx=16)
        bar.pack(fill="x")

        tk.Label(bar, text="⚡ Array Declaration Compiler",
                 font=FONT_BIG, bg=BG, fg=ACCENT).pack(side="left")

        btns = tk.Frame(bar, bg=BG)
        btns.pack(side="right")

        self._btn(btns, "📂 Load File",  self._load_file,    PANEL, FG).pack(side="left", padx=4)
        self._btn(btns, "💾 Save File",  self._save_file,    PANEL, FG).pack(side="left", padx=4)
        self._btn(btns, "📋 Example",    self._load_example, PANEL, FG).pack(side="left", padx=4)
        self._btn(btns, "🗑 Clear",      self._clear,        PANEL, FG).pack(side="left", padx=4)

        self.compile_btn = self._btn(btns, "▶  Compile",
                                     self._compile_click, BTN_BG, BTN_FG,
                                     font=FONT_HEAD)
        self.compile_btn.pack(side="left", padx=(12, 0))

    def _build_body(self):
        body = tk.PanedWindow(self, orient="horizontal",
                              bg=BG, sashwidth=6, sashrelief="flat")
        body.pack(fill="both", expand=True, padx=12, pady=(0, 6))

        # ── LEFT: source editor ──
        left = tk.Frame(body, bg=BG)
        body.add(left, minsize=300)

        tk.Label(left, text="SOURCE CODE", font=FONT_HEAD,
                 bg=BG, fg=FG_DIM).pack(anchor="w", pady=(2, 4))

        editor_frame = tk.Frame(left, bg=BORDER, bd=1, relief="flat")
        editor_frame.pack(fill="both", expand=True)

        self.editor = tk.Text(editor_frame, wrap="none",
                              font=FONT_CODE, bg=EDITOR_BG, fg=FG,
                              insertbackground=ACCENT, selectbackground=BORDER,
                              relief="flat", bd=8, undo=True,
                              tabs="4c")
        self.editor.pack(fill="both", expand=True)
        self.editor.bind("<Tab>", lambda e: (self.editor.insert("insert", "    "), "break"))

        # ── RIGHT: 3 output panels in a notebook ──
        right = tk.Frame(body, bg=BG)
        body.add(right, minsize=500)

        tk.Label(right, text="COMPILER OUTPUT", font=FONT_HEAD,
                 bg=BG, fg=FG_DIM).pack(anchor="w", pady=(2, 4))

        style = ttk.Style(self)
        style.theme_use("default")
        style.configure("Dark.TNotebook",        background=BG,    borderwidth=0)
        style.configure("Dark.TNotebook.Tab",    background=PANEL,  foreground=FG_DIM,
                        padding=[12, 4], font=FONT_UI)
        style.map("Dark.TNotebook.Tab",
                  background=[("selected", BORDER)],
                  foreground=[("selected", ACCENT)])

        nb = ttk.Notebook(right, style="Dark.TNotebook")
        nb.pack(fill="both", expand=True)

        self.token_out  = self._make_output_tab(nb, "Phase 1 — Tokens")
        self.ast_out    = self._make_output_tab(nb, "Phase 2 — Parse Tree")
        self.sem_out    = self._make_output_tab(nb, "Phase 3 — Semantics")
        self.raw_out    = self._make_output_tab(nb, "Raw Output")

        # set initial body sash position
        self.update_idletasks()
        body.sash_place(0, 420, 1)

    def _make_output_tab(self, nb, label):
        frame = tk.Frame(nb, bg=PANEL)
        nb.add(frame, text=label)

        txt = tk.Text(frame, wrap="word", state="disabled",
                      font=FONT_CODE, bg=PANEL, fg=FG,
                      relief="flat", bd=8, selectbackground=BORDER)

        sb = tk.Scrollbar(frame, command=txt.yview, bg=PANEL,
                          troughcolor=PANEL, activebackground=BORDER)
        txt.configure(yscrollcommand=sb.set)

        sb.pack(side="right", fill="y")
        txt.pack(fill="both", expand=True)

        # define colour tags
        txt.tag_configure("heading",  foreground=ACCENT,  font=FONT_HEAD)
        txt.tag_configure("token",    foreground=CYAN)
        txt.tag_configure("lexeme",   foreground=YELLOW)
        txt.tag_configure("loc",      foreground=FG_DIM)
        txt.tag_configure("ast_node", foreground=PURPLE)
        txt.tag_configure("ast_val",  foreground=PINK)
        txt.tag_configure("ast_meta", foreground=FG_DIM)
        txt.tag_configure("ok",       foreground=GREEN,   font=FONT_HEAD)
        txt.tag_configure("error",    foreground=RED,     font=FONT_HEAD)
        txt.tag_configure("rule",     foreground=YELLOW)
        txt.tag_configure("dim",      foreground=FG_DIM)
        txt.tag_configure("bold",     font=FONT_HEAD)

        return txt

    def _build_status_bar(self):
        bar = tk.Frame(self, bg=BORDER, height=28)
        bar.pack(fill="x", side="bottom")
        bar.pack_propagate(False)

        self.status_var = tk.StringVar(value="Ready — enter code and click Compile")
        tk.Label(bar, textvariable=self.status_var,
                 font=FONT_UI, bg=BORDER, fg=FG_DIM,
                 anchor="w", padx=10).pack(fill="both")

    # ── helpers ────────────────────────────────────────────────
    def _btn(self, parent, text, command, bg, fg, font=FONT_UI):
        return tk.Button(parent, text=text, command=command,
                         bg=bg, fg=fg, font=font,
                         relief="flat", cursor="hand2",
                         padx=10, pady=5,
                         activebackground=BORDER, activeforeground=FG)

    def _write(self, widget, text, tag=None):
        widget.config(state="normal")
        if tag:
            widget.insert("end", text, tag)
        else:
            widget.insert("end", text)
        widget.config(state="disabled")

    def _clear_widget(self, *widgets):
        for w in widgets:
            w.config(state="normal")
            w.delete("1.0", "end")
            w.config(state="disabled")

    def _set_status(self, msg, colour=FG_DIM):
        self.status_var.set(msg)
        # find label in status bar and recolour
        for child in self.winfo_children():
            if isinstance(child, tk.Frame) and child.cget("bg") == BORDER:
                for lbl in child.winfo_children():
                    lbl.config(fg=colour)
                break

    # ── actions ────────────────────────────────────────────────
    def _load_example(self):
        self.editor.delete("1.0", "end")
        self.editor.insert("end", EXAMPLE_CODE)
        self._clear_widget(self.token_out, self.ast_out, self.sem_out, self.raw_out)
        self._set_status("Example loaded — click ▶  Compile")

    def _clear(self):
        self.editor.delete("1.0", "end")
        self._clear_widget(self.token_out, self.ast_out, self.sem_out, self.raw_out)
        self._set_status("Ready")

    def _load_file(self):
        path = filedialog.askopenfilename(
            title="Open source file",
            filetypes=[("C files", "*.c"), ("All files", "*.*")])
        if not path:
            return
        try:
            self.editor.delete("1.0", "end")
            self.editor.insert("end", Path(path).read_text())
            self._set_status(f"Loaded: {path}")
        except Exception as e:
            messagebox.showerror("Load error", str(e))

    def _save_file(self):
        path = filedialog.asksaveasfilename(
            title="Save source file",
            defaultextension=".c",
            filetypes=[("C files", "*.c"), ("All files", "*.*")])
        if not path:
            return
        try:
            Path(path).write_text(self.editor.get("1.0", "end"))
            self._set_status(f"Saved: {path}")
        except Exception as e:
            messagebox.showerror("Save error", str(e))

    def _compile_click(self):
        if self._compile_running:
            return
        source = self.editor.get("1.0", "end").strip()
        if not source:
            self._set_status("No source code to compile.", RED)
            return
        self._compile_running = True
        self.compile_btn.config(state="disabled", text="⏳ Compiling…")
        self._set_status("Compiling…", ACCENT)
        self._clear_widget(self.token_out, self.ast_out, self.sem_out, self.raw_out)
        threading.Thread(target=self._run, args=(source,), daemon=True).start()

    # ── compiler execution ─────────────────────────────────────
    def _ensure_binary(self):
        if BINARY.exists():
            return
        r = subprocess.run(["make"], cwd=str(ROOT), capture_output=True, text=True)
        if r.returncode != 0 or not BINARY.exists():
            raise RuntimeError("Build failed:\n" + r.stdout + r.stderr)

    def _run(self, source):
        tmp = None
        try:
            self._ensure_binary()
            with tempfile.NamedTemporaryFile("w", suffix=".c",
                                             delete=False, encoding="utf-8") as f:
                f.write(source + "\n")
                tmp = f.name

            r = subprocess.run([str(BINARY), tmp],
                               capture_output=True, text=True)
            stdout = r.stdout
            stderr = r.stderr.strip()
            rc     = r.returncode

            self.after(0, self._display, stdout, stderr, rc)

        except Exception as e:
            self.after(0, self._display, "", str(e), -1)
        finally:
            if tmp:
                try:
                    os.unlink(tmp)
                except OSError:
                    pass

    # ── output rendering ───────────────────────────────────────
    def _display(self, stdout, stderr, rc):
        self._compile_running = False
        self.compile_btn.config(state="normal", text="▶  Compile")

        # ── raw tab ──
        self._write(self.raw_out, stdout)
        if stderr:
            self._write(self.raw_out, "\n" + stderr, "error")

        # ── parse sections ──
        self._render_tokens(stdout)
        self._render_ast(stdout)
        self._render_semantics(stdout, stderr, rc)

        # ── status bar ──
        if rc == 0:
            self._set_status("✔  ACCEPTED — no errors found", GREEN)
        elif rc == 1:
            self._set_status("✘  REJECTED — syntax error", RED)
        elif rc == 2:
            self._set_status("✘  REJECTED — semantic error", RED)
        else:
            self._set_status(f"✘  Error (exit {rc})", RED)

    # ── Phase 1 — Token table ──────────────────────────────────
    def _render_tokens(self, stdout):
        w = self.token_out
        w.config(state="normal"); w.delete("1.0", "end"); w.config(state="disabled")

        in_section = False
        rows = []
        for line in stdout.splitlines():
            if "PHASE 1" in line:
                in_section = True
                continue
            if "PHASE 2" in line:
                break
            if in_section:
                rows.append(line)

        if not rows:
            self._write(w, "No token output (possibly a very early error).\n", "dim")
            return

        self._write(w, "\n  Token Stream\n", "heading")
        self._write(w, "  " + "─" * 54 + "\n", "dim")

        # header
        self._write(w, f"  {'TYPE':<18} {'LEXEME':<14} LOCATION\n", "dim")
        self._write(w, "  " + "─" * 54 + "\n", "dim")

        token_re = re.compile(
            r'^\s{2}(\S+)\s+(\S+)\s+line\s+(\d+),\s+col\s+(\d+)')

        for row in rows:
            m = token_re.match(row)
            if m:
                ttype, lexeme, ln, col = m.groups()
                self._write(w, f"  ")
                self._write(w, f"{ttype:<18}", "token")
                self._write(w, f"{lexeme:<14}", "lexeme")
                self._write(w, f"line {ln}, col {col}\n", "loc")

        self._write(w, "\n")
        w.see("1.0")

    # ── Phase 2 — Parse Tree ──────────────────────────────────
    def _render_ast(self, stdout):
        w = self.ast_out
        w.config(state="normal"); w.delete("1.0", "end"); w.config(state="disabled")

        in_section = False
        rows = []
        for line in stdout.splitlines():
            if "PHASE 2" in line:
                in_section = True
                continue
            if "PHASE 3" in line:
                break
            if in_section:
                rows.append(line)

        if not rows:
            self._write(w, "No AST output.\n", "dim")
            return

        self._write(w, "\n  Parse Tree (AST)\n", "heading")
        self._write(w, "  " + "─" * 54 + "\n", "dim")

        node_re = re.compile(r'(\[(\w+)\])(.*)')
        meta_re = re.compile(r'(\(line \d+.*?\))')
        val_re  = re.compile(r'(type=\S+|name=\S+|size=\S+|op=\S+|\d+)')

        for row in rows:
            if not row.strip():
                self._write(w, "\n")
                continue

            m = node_re.search(row)
            if m:
                prefix  = row[:row.index(m.group(1))]
                node    = m.group(1)
                rest    = m.group(3)

                self._write(w, prefix, "dim")
                self._write(w, node,   "ast_node")

                # colour metadata (line/col) and values separately
                last = 0
                for mm in re.finditer(r'(\(line \d+.*?\))|(type=\S+|name=\S+|size=\S+|op=\S+|\s\d+)', rest):
                    self._write(w, rest[last:mm.start()])
                    txt = mm.group(0)
                    if txt.startswith("(line"):
                        self._write(w, txt, "ast_meta")
                    else:
                        self._write(w, txt, "ast_val")
                    last = mm.end()
                self._write(w, rest[last:] + "\n")
            else:
                self._write(w, row + "\n", "dim")

        w.see("1.0")

    # ── Phase 3 — Semantics ───────────────────────────────────
    def _render_semantics(self, stdout, stderr, rc):
        w = self.sem_out
        w.config(state="normal"); w.delete("1.0", "end"); w.config(state="disabled")

        in_section = False
        rows = []
        for line in stdout.splitlines():
            if "PHASE 3" in line:
                in_section = True
                continue
            if in_section:
                rows.append(line)

        # also show stderr errors
        err_lines = [l for l in stderr.splitlines() if l.strip()] if stderr else []

        self._write(w, "\n  Semantic Analysis\n", "heading")
        self._write(w, "  " + "─" * 54 + "\n", "dim")

        for row in rows:
            if "OK" in row or "no errors" in row.lower():
                self._write(w, "  " + row.strip() + "\n", "ok")
            elif "Error" in row or "error" in row:
                self._write(w, "  " + row.strip() + "\n", "error")
            elif row.strip():
                self._write(w, "  " + row.strip() + "\n")

        if err_lines:
            self._write(w, "\n  Errors / Warnings\n", "heading")
            self._write(w, "  " + "─" * 54 + "\n", "dim")
            for e in err_lines:
                self._write(w, "  " + e + "\n", "error")

        self._write(w, "\n")

        # big verdict
        if rc == 0:
            self._write(w, "\n  ✔  ACCEPTED\n", "ok")
        else:
            label = "Syntax Error" if rc == 1 else "Semantic Error" if rc == 2 else "Error"
            self._write(w, f"\n  ✘  REJECTED  ({label})\n", "error")

        # symbol table hint
        self._write(w, "\n  Semantic Rules Checked:\n", "dim")
        rules = [
            "1. Array declared before use",
            "2. Index must be integer type",
            "3. Literal index within bounds",
            "4. Initializer / assignment type compatibility",
            "5. Initializer count ≤ array size",
            "6. Array size must be positive",
        ]
        for r in rules:
            self._write(w, f"    • {r}\n", "rule")

        w.see("1.0")


# ── entry point ────────────────────────────────────────────────
if __name__ == "__main__":
    try:
        app = CompilerGUI()
        app.mainloop()
    except Exception as e:
        print(f"Startup error: {e}", file=sys.stderr)
        sys.exit(1)
