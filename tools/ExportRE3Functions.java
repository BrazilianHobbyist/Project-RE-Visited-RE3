/*
 * RE3Recomp — Script Ghidra para exportar funções do PS1 EXE
 * 
 * Como usar:
 *   1. Abra o executável do RE3 (SLUS_009.23) no Ghidra
 *   2. Vá em: Window > Script Manager
 *   3. Clique em "Script Directories" e adicione esta pasta
 *   4. Encontre "ExportRE3Functions.java" e clique em Run
 *   5. O script gera: seeds/functions.csv
 *
 * O CSV resultante tem o formato esperado pelo PSXRecomp:
 *   address,name,size
 *   0x80010000,entry_point,0x100
 *   ...
 */

// @category RE3Recomp
// @menupath Tools.RE3Recomp.Export Functions

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import java.io.*;

public class ExportRE3Functions extends GhidraScript {

    @Override
    public void run() throws Exception {
        FunctionManager fm = currentProgram.getFunctionManager();
        
        // Pedir ao usuário onde salvar o CSV
        File outFile = askFile("Salvar seeds em", "Salvar");
        if (outFile == null) {
            println("Cancelado.");
            return;
        }
        
        int count = 0;
        try (PrintWriter pw = new PrintWriter(new FileWriter(outFile))) {
            // Cabeçalho do CSV
            pw.println("address,name,size");
            
            // Iterar por todas as funções identificadas pelo Ghidra
            FunctionIterator iter = fm.getFunctions(true);
            while (iter.hasNext() && !monitor.isCancelled()) {
                Function func = iter.next();
                Address entry = func.getEntryPoint();
                String name = func.getName();
                long size = func.getBody().getNumAddresses();
                
                // Filtrar funções muito pequenas (provavelmente thunks/trampolins)
                if (size < 4) continue;
                
                // Formato: 0xADDRESS,nome,tamanho_em_bytes
                pw.printf("0x%08X,%s,%d%n", 
                    entry.getOffset(), 
                    name.replaceAll(",", "_"),  // escapar vírgulas no nome
                    size * 4                    // tamanho em bytes (4 bytes por instrução MIPS)
                );
                count++;
            }
        }
        
        println(String.format("✅ Exportadas %d funções para: %s", count, outFile.getPath()));
        println("Próximo passo: copie o arquivo para seeds/functions.csv no projeto RE3Recomp");
    }
}
