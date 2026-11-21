\# Bare-Metal



\## Criação de \*\*Drivers\*\* do flying\_fish



Operações essenciais para a criação básica de um dispositivo na pasta ```/dev```

```

\#include <linux/fs.h>         // struct file\_operations, struct file

\#include <linux/miscdevice.h> // struct miscdevice, misc\_register, misc\_deregister

\#include <linux/uaccess.h>    // copy\_to\_user, copy\_from\_user



// 1. OS MÉTODOS DE OPERAÇÃO (O que acontece quando o User Space chama read/ioctl)

static ssize\_t ff\_read(struct file \*filep, char \_\_user \*buf, size\_t count, loff\_t \*ppos) {

&#x20;   int dado\_kernel = 1;

&#x20;   if (copy\_to\_user(buf, \&dado\_kernel, sizeof(int)))

&#x20;       return -EFAULT;

&#x20;   return sizeof(int);

}



static long ff\_ioctl(struct file \*filep, unsigned int cmd, unsigned long arg) {

&#x20;   // Trata comandos de controle (ex: mutar buzzer)

&#x20;   return 0;

}



// 2. A TABELA DE LIGAÇÃO (Conecta as syscalls do User Space às suas funções)

static const struct file\_operations ff\_fops = {

&#x20;   .owner          = THIS\_MODULE,

&#x20;   .read           = ff\_read,

&#x20;   .unlocked\_ioctl = ff\_ioctl,

};



// 3. A IDENTIDADE DO DISPOSITIVO EM /dev/

static struct miscdevice ff\_misc = {

&#x20;   .minor = MISC\_DYNAMIC\_MINOR, // Pede um número Minor livre ao kernel

&#x20;   .name  = "flying\_fish",      // Cria exatamente o arquivo /dev/flying\_fish

&#x20;   .fops  = \&ff\_fops,           // Aponta para a tabela de operações acima

&#x20;   .mode  = 0666,               // Permissão rw-rw-rw- (acessível sem sudo)

};



// 4. ATIVAÇÃO E REMOÇÃO (Chamados no probe/init e remove/exit)

// Para criar o arquivo:    misc\_register(\&ff\_misc);

// Para remover o arquivo:  misc\_deregister(\&ff\_misc);





```



