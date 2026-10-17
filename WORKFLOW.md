# Protocolo de Governança e Plano de Trabalho Persistente
## Project RE: Visited — Resident Evil 3: Nemesis

Este documento estabelece o **Plano de Trabalho Íntegro, Auditável e Persistente** para todas as atualizações presentes e futuras do projeto. Qualquer modelo de IA ou desenvolvedor que atue neste workspace **DEVE** seguir rigorosamente as 6 etapas abaixo em cada ciclo de trabalho, sem omitir resultados, mascarar falhas ou deixar arquivos dessincronizados.

---

## Princípios Inegociáveis de Sinceridade e Organização

1. **Veracidade Estrita (Zero Alucinação de Status)**:
   - Nunca afirmar que uma funcionalidade está "implementada e validada" sem verificar:
     1. A existência do código no runtime (`RE3Recomp/psxrecomp/runtime/`) ou código gerado (`RE3Recomp/generated/`).
     2. O ponto exato de chamada ativa (*call site*) durante o loop de execução.
     3. A exposição no Launcher (`RE3Recomp/launcher/main.cpp`) ou arquivo de configuração (`settings.toml`).
     4. A compilação real refletida no binário final (`Resident_Evil_3__Nemesis_Recompiled.exe` e `Resident_Evil_3_Launcher.exe`).
2. **Fonte Única de Verdade (`RE3Recomp/`)**:
   - Todo desenvolvimento de código-fonte, patches e documentação ocorre prioritariamente dentro do repositório Git `RE3Recomp/`.
   - As pastas `ResidentEvil3-PCPort/` (ambiente de execução do usuário) e a raiz do workspace são sincronizadas automaticamente a partir de `RE3Recomp/` via script consolidado.
3. **Respeito à Política Zero-Asset**:
   - Jamais incluir arquivos proprietários da Capcom ou Sony (`SLUS_009.23`, imagens `.bin`/`.iso` do jogo, ou BIOS `SCPH*.BIN`) no repositório Git ou nos pacotes `.zip` de release. Apenas a `openbios.bin` (licença MIT) é distribuída.

---

## Ciclo Obrigatório de 6 Etapas para Cada Nova Atualização

### Etapa 1: Implementar (`Implement`)
- Realizar as alterações necessárias no runtime (`RE3Recomp/psxrecomp/`), nos gerados (`RE3Recomp/generated/`) ou no Launcher (`RE3Recomp/launcher/main.cpp`).
- Compilar o executável do jogo:
  ```powershell
  powershell -ExecutionPolicy Bypass -File "RE3Recomp\build.ps1" -Config Release
  ```
- Compilar o Launcher (caso alterado):
  ```powershell
  powershell -ExecutionPolicy Bypass -File "RE3Recomp\launcher\build.ps1"
  ```

### Etapa 2: Revisar e Auditar (`Review / Audit`)
- Executar a auditoria automatizada para validar paridade de hashes SHA-256, status do Git e relatórios de telemetria:
  ```powershell
  powershell -ExecutionPolicy Bypass -File "RE3Recomp\tools\manage_project.ps1" -Action audit
  ```
- Inspecionar `ResidentEvil3-PCPort/psx_last_run_report.json` e verificar se algum `psx_freeze_dump_*.json` foi gerado durante os testes.
- Distinguir claramente entre:
  - **Funcional e Ativo**: Compilado, conectado ao loop de execução e testado.
  - **Experimental / Em Código**: Presente no código mas desativado na UI ou aguardando validação.

### Etapa 3: Consolidar (`Consolidate`)
- Exportar o diff atualizado do submódulo `psxrecomp` para `RE3Recomp/patches/psxrecomp_runtime.patch` e sincronizar todos os binários e documentações entre `RE3Recomp/`, `ResidentEvil3-PCPort/` e a raiz do workspace:
  ```powershell
  powershell -ExecutionPolicy Bypass -File "RE3Recomp\tools\manage_project.ps1" -Action consolidate
  ```

### Etapa 4: Organizar (`Organize`)
- Garantir que nenhum arquivo temporário de debug, objeto solto ou dump antigo (`psx_freeze_dump_*.json`) permaneça nas pastas de distribuição ou no índice do Git.
- Verificar se os 3 executáveis do jogo e os 3 executáveis do Launcher possuem **exatamente o mesmo hash SHA-256**.

### Etapa 5: Registrar (`Record`)
- Atualizar `RE3Recomp/PROGRESS.md` (e sincronizar na raiz) com:
  - Data e hora real da atualização.
  - Lista transparente do que foi alterado, corrigido ou adicionado.
  - Hashes SHA-256 dos binários ativos.
  - Estado real do backlog e eventuais limitações conhecidas.
- Atualizar `README.md`, `README.pt-br.md` e `README.es.md` caso haja mudança de versão ou de funcionalidade visível ao usuário.

### Etapa 6: Atualizar GitHub e Fazer Novo Release (`Publish — Mediante Aprovação do Usuário`)
- **IMPORTANTE**: Nunca fazer `git push` ou publicar uma nova release sem a aprovação explícita do usuário após apresentar o relatório das Etapas 1 a 5.
- Uma vez aprovado pelo usuário:
  1. Gerar o pacote `.zip` limpo (Zero-Asset) para a nova versão:
     ```powershell
     powershell -ExecutionPolicy Bypass -File "RE3Recomp\tools\manage_project.ps1" -Action package -Version "vX.Y.Z"
     ```
  2. Realizar commit e tag no repositório `RE3Recomp`:
     ```powershell
     git -C "RE3Recomp" add -A
     git -C "RE3Recomp" commit -m "release: vX.Y.Z - <resumo objetivo>"
     git -C "RE3Recomp" tag -a "vX.Y.Z" -m "Release vX.Y.Z"
     git -C "RE3Recomp" push origin main --tags
     ```
