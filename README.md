# ForgeOS

**ForgeOS** é um sistema operacional x86_64 construído do zero, focado em aprendizado, experimentação e arquitetura limpa.

## Objetivos
- Construir um kernel real sem dependências de sistemas hospedeiros.
- Evoluir gradualmente de um boot simples até um sistema com userland, filesystem e GUI.
- Servir como material de estudo profundo sobre arquitetura de computadores e sistemas operacionais.

## Requisitos
- **Host:** Fedora Linux 44 KDE Plasma Edition
- **Emulador:** QEMU (x86_64)
- **Linguagem:** C++ (Freestanding) + Assembly (NASM)

## Compilação
```bash
make
```

## Execução
```bash
make run
```

## Debugging
```bash
make debug
```

## Estrutura de Diretórios
- `boot/`: Código Assembly de inicialização e transição de modo.
- `kernel/`: Lógica principal do kernel em C++.
- `include/`: Headers e tipos de dados isolados.
- `linker/`: Script de linker para mapeamento de memória.
- `scripts/`: Configurações do GRUB2.

## Roadmap
Veja a seção de Roadmap abaixo para as fases planejadas.

## Licença
MIT License. Veja [LICENSE](LICENSE) para mais detalhes.
