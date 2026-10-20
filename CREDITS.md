# Project RE: Visited - Resident Evil 3 — Créditos & Aviso Legal (Disclaimer)

---

## ⚖️ 1. Isenção de Responsabilidade Legal & Prevenção Jurídica (Legal Disclaimer)

### 1.1. Natureza Não Comercial e Finalidade de Preservação Histórica
Este projeto é o lançamento inicial da iniciativa **Project RE: Visited**, dedicada à preservação histórica e restauração moderna da trilogia clássica original de PlayStation (Resident Evil 1, Resident Evil 2 e Resident Evil 3). É uma iniciativa independente, de código aberto, conduzida por fãs e pesquisadores da comunidade retro (*fan-made project*), com propósito estritamente educacional, científico e de preservação digital de software histórico. **Não há qualquer finalidade lucrativa, cobrança, monetização, comercialização ou benefício financeiro** direta ou indiretamente associado a este software ou repositório.

### 1.2. Ativos e Dados Proprietários
Este repositório e o inicializador (*launcher*) **não distribuem** os seguintes arquivos pertencentes à **CAPCOM CO., LTD.** ou à **SONY INTERACTIVE ENTERTAINMENT INC.**:
* Imagens originais de disco de jogo (`.bin`, `.cue`, `.iso`, `.img`, `.mdf`);
* Trilhas sonoras oficiais, efeitos sonoros (*SFX*), gravações de voz e dublagens;
* Modelos tridimensionais, malhas poligonais e texturas de personagens ou cenários;
* Cenas cinematográficas pré-renderizadas (*FMVs* / vídeos);
* Cenários e artes de fundo pré-renderizadas;
* Textos narrativos, diálogos ou documentos originais de jogo;
* Dumps de BIOS proprietárias de varejo da Sony (ex: `SCPH1001.BIN`).

> **Observação:** o executável do jogo distribuído nas releases (`Resident_Evil_3__Nemesis_Recompiled.exe`) e os shards de overlay da pasta `cache/` contêm o código de máquina original do jogo traduzido automaticamente para x86-64, junto com os dados embutidos nesse código. Ele não inclui os arquivos de mídia do disco (áudio, vídeo, texturas, modelos, cenários), mas é derivado do programa original da Capcom.

> ⚠️ **REQUISITO OBRIGATÓRIO**: O usuário final deve obrigatoriamente possuir e fornecer seus próprios arquivos de jogo, extraídos de sua própria cópia física ou digital legalmente adquirida de *Resident Evil 3: Nemesis* (PlayStation 1, versão SLUS-00923), para que o executável e seus recursos possam operar.

### 1.3. Engenharia Reversa Limpa e Recompilação Estática para Interoperabilidade
O projeto usa recompilação estática automatizada: o código de máquina do jogo é traduzido para C pelo recompilador do PSXRecomp, não reescrito à mão, e roda sobre o runtime de código aberto do PSXRecomp, que emula o hardware do PlayStation. Não se trata de uma reimplementação *clean-room*: o resultado é derivado do programa original. O objetivo é possibilitar a execução e interoperabilidade de um programa de computador legítimo em sistemas operacionais e arquiteturas modernas (Windows / x86_64).

Essa metodologia encontra pleno amparo em legislações e precedentes jurídicos nacionais e internacionais:
* **Estados Unidos**: *Digital Millennium Copyright Act* (DMCA), 17 U.S.C. § 1201(f) (*Reverse Engineering for Interoperability Exception*), e jurisprudência consolidada dos Tribunais Federais: *Sega Enterprises Ltd. v. Accolade, Inc.* (977 F.2d 1510) e *Sony Computer Entertainment, Inc. v. Connectix Corp.* (203 F.3d 596).
* **União Europeia**: Diretiva 2009/24/CE do Parlamento Europeu e do Conselho, de 23 de abril de 2009, Artigo 6º (*Descompilação com o objetivo estrito de alcançar a interoperabilidade de programas de computador criados de forma independente*).
* **Brasil**: Lei nº 9.609/1998 (Lei do Software), Artigo 6º, inciso I (*Não constitui ofensa aos direitos do titular de programa de computador a operação necessária para a obtenção de informações indispensáveis à interoperabilidade de programa de computador criado de forma independente*).

### 1.4. Reconhecimento de Marcas Registradas (*Trademarks Acknowledgment*)
* **"Resident Evil"**, **"Biohazard"**, **"Nemesis"**, **"Jill Valentine"**, **"Umbrella"** e todos os títulos, logotipos, personagens e elementos temáticos correlatos são marcas comerciais registradas de propriedade exclusiva da **CAPCOM CO., LTD.**
* **"PlayStation"**, **"PS1"**, **"PSX"**, **"DualShock"** e logotipos associados são marcas comerciais registradas da **SONY INTERACTIVE ENTERTAINMENT INC. (SIE)**.
* A utilização dessas marcas e nomenclaturas neste projeto tem caráter exclusivamente referencial, descritivo e identificativo (*nominative fair use*), visando estritamente informar a compatibilidade do software, não implicando qualquer afiliação, endosso, patrocínio, aprovação ou licença oficial por parte da Capcom Co., Ltd. ou Sony Interactive Entertainment Inc.

### 1.5. Termo de Isenção de Garantias e Limitação de Responsabilidade (*AS IS*)
ESTE SOFTWARE É FORNECIDO "NO ESTADO EM QUE SE ENCONTRA" (*AS IS*), SEM QUALQUER GARANTIA EXPRESSA OU IMPLÍCITA, INCLUINDO, MAS NÃO SE LIMITANDO A, GARANTIAS DE COMERCIABILIDADE, ADEQUAÇÃO A UMA FINALIDADE ESPECÍFICA OU NÃO VIOLAÇÃO. EM NENHUMA HIPÓTESE OS AUTORES, DESENVOLVEDORES OU CONTRIBUIDORES SERÃO RESPONSABILIZADOS POR QUAISQUER RECLAMAÇÕES, PERDAS, DANOS DIRETOS, INDIRETOS, INCIDENTAIS OU CONSEQUENCIAIS DECORRENTES DO USO OU DA INCAPACIDADE DE USO DESTE SOFTWARE.

---

## 🎮 2. Créditos Completos de Ferramentas, Projetos e Fontes

Expressamos nossa admiração, respeito e sincero agradecimento aos criadores originais e a todas as ferramentas, bibliotecas e comunidades de código aberto que tornaram este projeto viável:

### 1. Obra Original & Plataforma
* **[CAPCOM CO., LTD.](https://www.capcom.com/)**: Pela criação da obra-prima seminal *Resident Evil 3: Nemesis* (*Biohazard 3: Last Escape*, 1999). Direção de Kazuhiro Aoyama, produção de Shinji Mikami e trilha sonora inesquecível de Masami Ueda, Saori Maeda e Shusaku Uchiyama.
* **[SONY INTERACTIVE ENTERTAINMENT](https://www.playstation.com/)**: Pelo desenvolvimento da lendária arquitetura de hardware do PlayStation original (1994), processador MIPS R3000A (LSI Logic Core), coprocessador GTE (Geometry Transformation Engine), GPU de 24 bits e SPU ADPCM.

### 2. Projetos de Inspiração & Preservação de Survival Horror
* **[Silent Hill (PS1) Decompilation & Native PC Port](https://github.com/shdecompilations/silent-hill-decomp)**: Pelo trabalho inspirador e pioneiro da comunidade na descompilação de *Silent Hill* (PS1) (`shdecompilations/silent-hill-decomp`, iniciado por Vatuu e colaboradores) e pelos projetos derivados de port nativo para PC (`sh1pc.com`, SlickAmogus e comunidade). O sucesso da comunidade de engenharia reversa em trazer a atmosfera única e aterrorizante de Silent Hill nativamente para computadores modernos — com suporte a telas widescreen, altas taxas de quadros e controles refinados — serviu como inspiração direta, modelo técnico e grande motivação para o desenvolvimento deste port de Resident Evil 3.
* **[N64: Recompiled / Zelda64: Recompiled](https://github.com/Mr-Wiseguy/N64Recomp)**: Criado por Mr-Wiseguy e equipe. Os pioneiros da tecnologia moderna de recompilação estática limpa (*asset-free*) de consoles retro, que estabeleceram a base teórica e ética adotada por este projeto.

### 3. Framework de Recompilação & Ecossistema Retro
* **[PSXRecomp (RetroPortingToolkit)](https://github.com/RetroPortingToolKit/psxrecomp)**: Criado por Matthew Stanley ("mstan") e os desenvolvedores do coletivo RetroPortingToolkit. Framework central de recompilação estática de PlayStation 1 que traduz código de máquina MIPS R3000A para C nativo, com runtime LLE/HLE dual e gerenciamento de hardware. Licença: *PolyForm Noncommercial 1.0.0*.

### 4. Engenharia Reversa & Análise de Binários
* **[Ghidra SRE](https://ghidra-sre.org/)**: Desenvolvido pela Diretoria de Pesquisa da National Security Agency (NSA). Plataforma aberta de engenharia reversa utilizada para desmonte de código, descompilação de rotinas e identificação sistemática das 3.328 funções do binário SLUS_009.23 (`seeds/ghidra_funcs.txt` via `ExportRE3Functions.java`). Licença: *Apache 2.0*.
* **[Rabbitizer](https://github.com/Decompollaborate/rabbitizer)**: Desenvolvido pelo time Decompollaborate. Desmontador e analisador de alta performance de instruções de arquitetura MIPS. Licença: *MIT*.
* **[ELFIO](https://github.com/serge1/ELFIO)**: Desenvolvido por Serge Lamikhov-Center. Biblioteca C++ para leitura e geração de formatos binários ELF. Licença: *MIT*.

### 5. BIOS Aberta & Oráculos de Emulação
* **[PCSX-Redux & OpenBIOS](https://github.com/grumpycoders/pcsx-redux)**: Criado por grumpycoders e colaboradores do PCSX-Redux. Reimplementação 100% limpa (*clean-room*) do BIOS do PS1 em código aberto, possibilitando inicialização nativa legal sem exigir dumps comerciais de BIOS. Licença: *MIT*.
* **[Beetle PSX / Mednafen / Libretro](https://github.com/libretro/beetle-psx-libretro)**: Desenvolvido pela equipe Mednafen (Ryphecha) e comunidade Beetle PSX / Libretro. Emulador de precisão de ciclos utilizado como oráculo comparativo e padrão-ouro de fidelidade de hardware. Licença: *GPL-2.0*.
* **[DuckStation](https://github.com/stenzek/duckstation)**: Desenvolvido por stenzek. Emulador moderno de referência técnica para compatibilidade de cartões de memória (`.mcd`), pipeline de renderização e avanços PGXP. Licença: *GPL-3.0*.

### 6. Melhorias Gráficas, Precisão Geométrica & Shaders
* **[PGXP (Parallel GPU Extended Precision)](https://github.com/iCatButler)**: Criado por iCatButler, com evoluções contínuas de simias, Pete Bernert, stenzek e comunidade Beetle PSX. Tecnologia de coordenadas sub-pixel em ponto flutuante e correção de perspectiva de texturas que elimina a oscilação poligonal (*polygon jitter*) e as deformações angulares clássicas do PS1.
* **[Khronos Group & OpenGL](https://www.khronos.org/opengl/)**: Especificação padrão da indústria para aceleração gráfica 3D por hardware, shaders de apresentação em tempo real e integração opcional com OpenXR SDK.
* **Shaders Customizados**: Filtro FXAA (*Fast Approximate Anti-Aliasing*) e escalonamento bicúbico Catmull-Rom para renderização interna em alta definição (1080p, 1440p e 4K Ultra HD).

### 7. Áudio & Ciência de Cores
* **[JRickey (gba-recomp / snesrecomp)](https://github.com/JRickey/gba-recomp)**: Desenvolvido por JRickey. Algoritmo *ShadowVerifier* (autocalibração diferencial de ganho e correlação de envoltória de áudio) e matrizes de calibração de cores de tela (xyY→XYZ, adaptação cromática de Bradford e perfis de fósforo Trinitron/CRT). Licença: *MIT / Apache-2.0*.
* **SPU HQ 48kHz Resampling**: Subsistema de interpolação cúbica de 4 pontos em ponto flutuante para reprodução de áudio SPU ADPCM com máxima fidelidade.

### 8. Bibliotecas de Terceiros (*Third-Party Dependencies*)
* **[SDL2 (Simple DirectMedia Layer)](https://www.libsdl.org/)**: Criado por Sam Lantinga e comunidade. Gerenciamento de janelas, contexto OpenGL, temporização de quadros e suporte a controles modernos via XInput e DirectInput. Licença: *zlib License*.
* **[libchdr](https://github.com/rtissera/libchdr)**: Criado por Romain Tisserand. Biblioteca de descompressão de imagens de disco no formato comprimido CHD. Licença: *BSD-3-Clause*.
* **Bibliotecas de Compressão**:
  * **Zstandard (zstd)**: Yann Collet / Meta (*BSD-3-Clause*);
  * **LZMA SDK**: Igor Pavlov (*Domínio Público*);
  * **miniz**: Rich Geldreich (*MIT*).
* **[TinyCC (TCC)](https://bellard.org/tcc/)**: Criado por Fabrice Bellard. Compilador C ultrarrápido utilizado pelo pipeline de sharding AOT para compilação em segundo plano de overlays de código em DLLs nativas sem necessidade de ambiente de desenvolvimento instalado. Incluído em `overlay_toolchain/tcc/` (versão 0.9.27, aviso de licença em `COPYING`). Licença: *LGPL-2.1*.
* **[Python](https://www.python.org/)**: Python Software Foundation. Distribuição embutida 3.13.1 incluída em `overlay_toolchain/python/` para executar o compilador de overlays na máquina do jogador. Licença: *PSF License* (arquivo `LICENSE.txt` incluído).
* **[toml11](https://github.com/ToruNiina/toml11)**: Criado por Toru Niina. Biblioteca C++ para leitura e escrita de arquivos no formato TOML (`game.toml`, `settings.toml`). Licença: *MIT*.
* **[stb_image & stb libraries](https://github.com/nothings/stb)**: Criado por Sean T. Barrett (*nothings.org*). Bibliotecas *header-only* em C/C++ para carregamento de texturas e imagens. Licença: *MIT / Domínio Público*.
* **[{fmt}](https://github.com/fmtlib/fmt)**: Criado por Victor Zverovich. Biblioteca de formatação moderna e segura de strings em C++. Licença: *MIT*.
* **[retcomm-studio (Retro-Studio)](https://github.com/RetroPortingToolKit/Retro-Studio)**: Criado por Alex Vanderveen. Ferramental de análise e validação de imagens multi-disco. Licença: *MIT*.
* **[libjuice](https://github.com/paullouisageneau/libjuice)**: Criado por Paul-Louis Ageneau. Biblioteca de protocolo de transporte ICE para conexão de rede P2P (*Netplay*). Licença: *MPL-2.0*.

### 9. Toolchain & Ambiente Operacional
* **[MinGW-w64](https://www.mingw-w64.org/) & [GCC (GNU Compiler Collection)](https://gcc.gnu.org/)**: Toolchain de compilação C/C++ utilizada para compilar o executável de 64 bits para Windows e o launcher nativo.
* **[Microsoft Corporation](https://www.microsoft.com/)**: Win32 API, biblioteca GDI+ e biblioteca Common Controls v6 (ComCtl32) que dão vida à interface gráfica do Launcher.
* **[CMake](https://cmake.org/) & [Ninja](https://ninja-build.org/)**: Sistema de automação de build e geração de projetos.

### 10. Comunidade & Colaboradores
* **Comunidade R.A.I.D. (Retro AI Development)**: Pelo espaço de discussão técnica, engenharia reversa assistida por inteligência artificial e incentivo à preservação de clássicos dos videogames.

### 11. Recursos e Melhorias Exclusivas Deste Port
* **Assistente de Instalação com Extração Direta ISO 9660**: Leitor de setores Mode 2 Form 1 que extrai automaticamente o executável `SLUS_009.23` de imagens `.bin`/`.iso` sem necessidade de programas de terceiros.
* **Controles Modernos 3D**: Movimentação analógica em 360 graus relativa à câmera com corrida automática (*auto-run*) no analógico esquerdo, mantendo integralmente o controle tanque clássico no D-Pad.
* **Interpolação Poligonal Temporal Contínua**: Fluidez total sincronizada a altas taxas de atualização (60Hz, 120Hz, 144Hz, 165Hz, 240Hz ou sincronização nativa com o monitor).
* **Launcher Gráfico Nativo em Win32/GDI+**: Painel de configurações visual em tempo real para resolução interna, proporção de tela, filtros, controles e áudio.
* **Pular Cutscenes e FMVs (Auto-Skip)**: Bypass instantâneo de cenas em vídeo MDEC e áudio XA via botão de ação ou configuração.
* **Preservação de Saves Nativos**: Estrutura de cartões de memória PS1 mantida em arquivos `.mcd` binários reais, compatíveis com consoles físicos e outros emuladores.

---
*Project RE: Visited - Resident Evil 3 — Um tributo à história dos videogames e à engenharia de software de preservação.*
