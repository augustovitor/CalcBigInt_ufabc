# CalcBigInt — Calculadora de Inteiros de Precisão Arbitrária (ANSI C)

## 🧩 Visão Geral

**CalcBigInt** é uma calculadora em C capaz de operar com **inteiros gigantes**, com quantidades de dígitos muito maiores do que os tipos nativos (`int`, `long long`, etc.) suportam.
Os números são manipulados usando uma **estrutura de Big Integer**, construída manualmente em ANSI C.

O projeto foi desenvolvido como parte da disciplina **Programação Estruturada — UFABC** e demonstra conhecimentos em:

* Estruturas de dados
* Manipulação de memória (heap)
* Algoritmos de aritmética de precisão arbitrária
* Modularização de código em C
* Entrada e saída via arquivos e terminal
* Organização profissional de repositório

---

## Funcionalidades

### **Operações implementadas (base):**

* Soma (`+`)
* Subtração (`-`)
* Multiplicação (`*`)
* Divisão inteira (`/`)
* Módulo (`%`)

### **Operação personalizada**

O projeto permite adicionar uma operação extra mais complexa, como:

* Fatorial
* MDC
* Fatoração
* XOR entre números grandes
* Multiplicação de matrizes


### **Modos de entrada/saída**

* **Modo Interativo:** números digitados pelo usuário
* **Modo Arquivo:** leitura automática e saída gerada em arquivo novo

---

## Estrutura do Projeto

```
calc-bigint/
├── src/
│   ├── main.c
│   ├── bigint.c
│   ├── bigint.h
│   ├── operations.c
│   ├── operations.h
│   ├── io.c
│   └── io.h
│
├── exercises/                 # Mini-tarefas do professor (treinamento)
│   ├── tarefa_01_menu/
│   ├── tarefa_02_soma_digito/
│   └── tarefa_03_random_seed/
│
├── examples/
├── tests/
├── Makefile
├── LICENSE
├── README.md
└── .gitignore
```

### **Principais módulos**

🟦 **bigint.c / bigint.h** — criação, destruição, parsing e impressão de BigInts

🟩 **operations.c / operations.h** — operações aritméticas entre BigInts

🟨 **io.c / io.h** — leitura e escrita (terminal/arquivos)

🟥 **main.c** — loop da calculadora, sistema de menu

---

## Como a Aritmética Funciona

A implementação usa **blocos de 9 dígitos (base 1e9)** para representar números muito grandes.

Exemplo:

```
Número: 12345678901234567890
Representação:
[34567890][234567890][12]
```

Isso deixa a soma e multiplicação mais rápidas e reduz o número de iterações.

As operações seguem o mesmo algoritmo que fazemos no papel, mas com blocos:

* **Soma:** propaga carry bloco a bloco
* **Subtração:** usa “empréstimo” entre blocos
* **Multiplicação:** método escolar otimizado (ou Karatsuba opcional)
* **Divisão:** método de tentativa com aproximação (divisão longa)

---

## Como Compilar

Certifique-se de ter `gcc` instalado.
Depois, basta rodar:

```bash
make
```

O binário será gerado em:

```
/bin/calcbigint
```

---

## Testes

O diretório `tests/` contém testes básicos, e você pode criar novos usando:

```bash
./bin/calcbigint
```

E fornecendo entradas manualmente.

Também há um script simples:

```bash
sh tests/minimal_tests.sh
```

---

## 📁 Exercícios Preparatórios

O diretório `exercises/` contém as mini-tarefas que fundamentam a construção da calculadora:

* **Menu simples de operações**
* **Soma dígito a dígito com carry** (base da soma de BigInts)
* **Gerador de números grandes com seed** (útil para testar)

Esses exercícios demonstram o aprendizado progressivo e reforçam conceitos importantes da disciplina.

---

## 👨‍💻 Como Executar

### **Modo Interativo:**

```bash
./bin/calcbigint
```

Escolha uma operação e digite dois números grandes.

### **Modo Arquivo:**

Você pode criar um arquivo com os operandos e deixar o programa gerar outro com a resposta.

---

## 🧱 Melhorias Futuras

* Implementação completa de subtração com sinais
* Multiplicação otimizada (Karatsuba / FFT)
* Divisão mais rápida
* Suporte a números negativos em todas as operações
* Interface gráfica simples (GTK ou ImGui)

---

## 📄 Licença

Este projeto é distribuído sob licença **MIT**.

---

## 🤝 Contribuições

Sugestões, issues e pull requests são bem-vindos. Este repositório foi estruturado para servir como portfólio profissional e demonstração de domínio em C.

---

## 📫 Contato

Se quiser conversar sobre o projeto, otimizações ou estrutura de dados:

* **Autor:** augustovitor
* **GitHub:** [https://github.com/augustovitor](https://github.com/augustovitor)

---


