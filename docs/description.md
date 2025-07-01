# Practical Assignment - Intel SGX

Unlimited attempts allowed

## Assignment

This assignment consists of 5 challenges, each worth 2 points. In the home folder of your virtual environment, you will
find two files:

- `enclave-atividade.signed.so`: An enclave created by the professor, which you will have to interact with.
- `enclave-atividade.edl`: The interface with all the **OCALLS** and **ECALLS** exposed by this enclave.

You can also download the Enclave here:

- [enclave-atividade.edl](./enclave-atividade.edl)
- [enclave-atividade.signed.so](./enclave-atividade.signed.so)

You do not have access to the source code of this enclave (and it is not necessary), but you will have to interact with
it to complete the challenges.

1. First, log into the virtual environment by following this guide:
   [Guide for using the virtual laboratory.pdf](./Guia%2520para%2520utiliza%C3%A7%C3%A3o%2520do%2520laborat%C3%B3rio%2520virtual%2520.pdf)
2. Log into the virtual machine using the guide, and clone the base project
   [https://github.com/Lohann/intel-sgx-template](https://github.com/Lohann/intel-sgx-template)
3. Carefully read the instructions in the base project's
   [README.md](https://github.com/Lohann/intel-sgx-template?tab=readme-ov-file#project-structure), and compile the
   project to ensure that everything is working as it should.
4. Now that you know the project is working, copy the `enclave-atividade.signed.so` file to the root of the project.
5. Change the **ENCLAVE_FILENAME** constant in the `app/app.h` file to load `enclave-atividade.signed.so`.
6. Replace the content of the `enclave/enclave.edl` file with the content of the provided `enclave-atividade.edl` file.
7. Run `make clean` to remove the compiled files and `make SGX_MODE=SIM main` to compile **only the application; it is
   no longer necessary to compile the Enclave.**
8. Run `./main` to verify that the project is working.
9. Done! Now you are ready to start the challenges.

## Challenges

### 1. Call the enclave

Modify the project to call the `ecall_verificar_aluno` ecall from the `enclave-atividade.signed.so` enclave. It receives
a string as a parameter.

### 2. Crack the password

The `ecall_verificar_senha` ecall receives an integer as a parameter that represents a password. If you provide the
correct password, it will return **0**, or **-1** if you are incorrect.

The password is a random integer of up to 5 digits, meaning a number between **0** and **99999**. Your task is to
discover the password.

### 3. Secret Sequence

The `ecall_palavra_secreta` ecall receives a `char[20]` array of 20 letters as a parameter. The enclave has a **sequence
of 20 secret letters** stored within it, composed only of **uppercase letters**. Every time you call this enclave
passing an array of letters, it will replace ALL the letters you got wrong in your array with the character `-`
(hyphen), but it will leave the letters in the positions you got right intact. Example:

- You called **ecall_palavra_secreta** with the character array `ABCDEZHIKLMNOPQRSTVX`
- The enclave changed the array, and now it is equal to `---DE--I---N------V-`, which means you got 5 letters correct.
- You kept the letters you got right and now sent `QAEDESEIIUJNLUMMWDVJ`
- The enclave changed the array, and now it is equal to `---DE--I---N----W-V-`. You found a new letter!
- And so on, until you discover the entire secret phrase.

### 4. Secret Polynomial

The `ecall_polinomio_secreto` ecall receives a number `x` as a parameter and returns the result of the quadratic
equation $(ax^2 + bx + c) % p$, where $a$, $b$, and $c$ are secret values and $p$ is the prime number
[2147483647](https://en.wikipedia.org/wiki/2,147,483,647) (the largest 32-bit prime). You can call this ocall with any
value except zero; if you pass zero, the call will fail. Your task is to discover $a$, $b$, and $c$.

### 5. Rock, Paper, Scissors

- [enclave-desafio-5.edl](./enclave-desafio-5.edl)
- [enclave-desafio-5.signed.so](./enclave-desafio-5.signed.so)

Replace the project's **enclave-atividade.signed.so** and **enclave-atividade.edl** with the ones provided above. You do
not need to create a new project. The enclave for this challenge has the same interface as the previous one; it just
adds two functions:

- `ecall_pedra_papel_tesoura`
- `ocall_pedra_papel_tesoura`

In the `app/app.c` code, create and implement the `ocall_pedra_papel_tesoura` ocall. This ocall will be called by the
**enclave** when you call the `ecall_pedra_papel_tesoura` ecall. The enclave will call your ocall 20 times, and it must
always return an integer between 0 and 2, where **rock = 0**, **paper = 1**, and **scissors = 2**. (Note: if you do not
implement this ocall, the execution will fail). At the end of the execution, the enclave will return an integer
representing how many matches you won, or a **negative value** if any problem occurred. You must win all 20 matches to
complete the challenge. The submission is all of the enclave's plays and your plays, which will be printed to the
terminal when the challenge is completed. Example:

```raw
------------------------------------------------
[ENCLAVE] DESAFIO 5 CONCLUIDO!!
          ENCLAVE JOGADAS: 11212201212200121120
             SUAS JOGADAS: 22020012020011202201
------------------------------------------------
```
