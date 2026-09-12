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
