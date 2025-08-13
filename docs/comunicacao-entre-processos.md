# Comunicação entre processos - Barreiras, memória transacional e troca de mensagens

https://drive.google.com/file/d/1QLKGKAxq4k58E3zhhO5bfEFajAG8i2gm/view

### Caso especial do mutex

- da pra implementar a partir do mutex
- quando chega o quarto processo todo mundo pode continuar a execucao
- "ponto de parada"

```
function barrier_wait (barreira, processo)
{
    atomic {
        barreira.contador = barreira.contador + 1
        if (barreira.contador < barreira.numero_processos)
        
        dormir(processo)
        
        else
        
        acordar_todos_processos()
    }
}
```

- da pra ter universal ou pra um número de processos/threads 

### Memória transacional

- comunicação implícita - duas threads acessando o mesmo endereço
- shared memory communication
- mas tem a explícita - enviar uma mensagem pra thread 1 - passagem de mensagens
- multicore usando threads 

### Diferença de threads e Processos

- Threads tem acesso a mesma memória 
- Cada processo tem seu próprio espaço de acesso a memória