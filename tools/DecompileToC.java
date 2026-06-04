// Ghidra headless 後處理:把所有函式反編成單一 C 檔當 oracle 參考 + 函式索引
// @category Ultima2
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.util.task.ConsoleTaskMonitor;
import java.io.PrintWriter;

public class DecompileToC extends GhidraScript {
    @Override
    public void run() throws Exception {
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        ConsoleTaskMonitor monitor = new ConsoleTaskMonitor();

        PrintWriter fc = new PrintWriter("/work/decompile/out/ultima2_decompiled.c");
        PrintWriter fi = new PrintWriter("/work/decompile/out/functions_index.tsv");
        fc.println("/* Ultima2.exe Ghidra 12.1 decompilation (oracle reference, NOT for compiling) */\n");
        fi.println("name\tentry\tsize_bytes\tdecompiled");

        FunctionManager fm = currentProgram.getFunctionManager();
        int ok = 0, fail = 0;
        for (Function func : fm.getFunctions(true)) {
            String name = func.getName();
            String entry = func.getEntryPoint().toString();
            long size = func.getBody().getNumAddresses();
            DecompileResults res = decomp.decompileFunction(func, 120, monitor);
            if (res.decompileCompleted()) {
                fc.println("/* ===== " + name + " @ " + entry + " (" + size + " bytes) ===== */");
                fc.println(res.getDecompiledFunction().getC());
                fc.println();
                fi.println(name + "\t" + entry + "\t" + size + "\tY");
                ok++;
            } else {
                fc.println("/* FAILED to decompile " + name + " @ " + entry + " */\n");
                fi.println(name + "\t" + entry + "\t" + size + "\tN");
                fail++;
            }
        }
        fc.close();
        fi.close();
        println("[DecompileToC] done: " + ok + " ok, " + fail + " failed, total " + (ok + fail));
    }
}
