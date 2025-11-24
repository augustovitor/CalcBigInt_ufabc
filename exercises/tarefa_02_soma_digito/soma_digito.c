#include <stdio.h>
#include <string.h>

int main(void) {
    char a[100], b[100];
    int i, j, carry = 0, soma, d1, d2;
    char resultado[101];
    int k = 0;

    printf("Digite o valor de a: ");
    scanf("%s", a);
    printf("Digite o valor de b: ");
    scanf("%s", b);

    i = strlen(a) - 1;
    j = strlen(b) - 1;

    printf("\nd1 d2 Soma Cout\n");

    while (i >= 0 || j >= 0) {
        d1 = (i >= 0) ? a[i] - '0' : 0;
        d2 = (j >= 0) ? b[j] - '0' : 0;

        soma = d1 + d2 + carry;

        // Cout é o vai-um
        carry = soma / 10;
        int s = soma % 10;

        printf("%d %d %d %d\n", d1, d2, s, carry);

        resultado[k++] = s + '0';

        i--;
        j--;
    }

    if (carry > 0)
        resultado[k++] = carry + '0';

    // inverter resultado
    for (i = 0; i < k / 2; i++) {
        char temp = resultado[i];
        resultado[i] = resultado[k - 1 - i];
        resultado[k - 1 - i] = temp;
    }
    resultado[k] = '\0';

    printf("Resultado: %s\n", resultado);
    return 0;
}