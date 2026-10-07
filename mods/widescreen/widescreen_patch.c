/*
 * RE3Recomp — Widescreen 16:9 Patch
 * 
 * RE3 usa o GTE (Geometry Transformation Engine) do PS1 para renderização 3D.
 * A constante de aspect ratio original é 320/240 (4:3).
 * Este patch intercepta a configuração da projeção para usar 16:9.
 *
 * ATENÇÃO: Os endereços de memória abaixo (TODO) precisam ser encontrados
 * via Ghidra analisando as chamadas às instruções cop2 (GTE) que configuram
 * a distância focal e o offset de tela.
 *
 * Referência GTE relevante:
 *   - OFX / OFY : offsets de tela (center)
 *   - H         : distância focal (projective distance)
 *   - DQA / DQB : coeficientes de profundidade
 */

#include "psxrecomp/runtime/cpu_context.h"
#include "psxrecomp/runtime/gte.h"

/* Aspect ratio 16:9 vs. original 4:3 */
#define ASPECT_ORIG  (4.0f / 3.0f)
#define ASPECT_WIDE  (16.0f / 9.0f)
#define WIDE_SCALE   (ASPECT_WIDE / ASPECT_ORIG)   /* ≈ 1.3333 */

/*
 * __re3_patch_gte_projection()
 * 
 * Esta função é chamada como "stub override" pelo runtime PSXRecomp
 * toda vez que o jogo configura os registradores GTE de projeção.
 *
 * TODO (Fase 2 — Ghidra):
 *   Identificar a função original que escreve em OFX/OFY/H do GTE.
 *   Registrar o endereço em game.toml > [recompiler] > stubs:
 *     "0x8001XXXX" = "re3_patch_gte_projection"
 */
void __re3_patch_gte_projection(CpuContext* ctx) {
    /* Ler o valor de H (distância focal) que o jogo quer setar */
    uint16_t original_h = (uint16_t)(ctx->r5 & 0xFFFF);

    /*
     * Para expandir o campo de visão horizontal sem esticar a imagem,
     * aumentamos H proporcionalmente. Isso faz com que mais geometria
     * seja visível nas laterais da tela em 16:9.
     *
     * Nota: Os fundos pré-renderizados de RE3 são fixos em 4:3 e
     * continuarão com barras laterais pretas. Apenas os modelos 3D
     * (Jill, Nemesis, inimigos, objetos) terão FOV expandido.
     */
    uint16_t wide_h = (uint16_t)((float)original_h / WIDE_SCALE);

    /* Escrever o H corrigido no registrador GTE */
    ctx->cop2_ctrl[26] = wide_h;   /* GTE_H register */

    /*
     * OFX e OFY (offsets de centro de tela) permanecem no centro.
     * O runtime PSXRecomp já ajusta a viewport para 1920x1080.
     */
}

/*
 * Registro do patch no sistema de mods do PSXRecomp.
 * Esta função é chamada automaticamente ao carregar o mod.
 */
void re3_widescreen_init(void) {
    /* 
     * TODO: Substituir 0x8001XXXX pelo endereço real encontrado no Ghidra.
     * O stub_register mapeia um endereço de função PS1 para nossa implementação C.
     */
    // psxrecomp_stub_register(0x8001XXXX, __re3_patch_gte_projection);
}
