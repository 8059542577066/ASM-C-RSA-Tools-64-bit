#include <stdio.h>
#include <string.h>


void main()
{
    int size;
    printf("Enter RSA Key Size (Min 1024): ");
    scanf("%d", &size), fseek(stdin, 0, SEEK_END);

    if (size >= 1024)
    {
        unsigned int amount, i;
        printf("Enter Number of Key Pairs to Create: ");
        scanf("%u", &amount), fseek(stdin, 0, SEEK_END);

        for (i = 0; i != amount; ++i)
        {
            char pubName1[22], priName1[23], pubName8[26], priName8[27], n[11];
            strcpy(pubName1, "RSA "), strcpy(priName1, "RSA ");
            strcpy(pubName8, "Public Key "), strcpy(priName8, "PRIVATE KEY ");
            sprintf(n, "%u", i + 1), printf("\n%s.\n", n);
            strcat(pubName8, n), strcat(pubName8, ".txt");
            strcat(priName8, n), strcat(priName8, ".txt");
            strcat(pubName1, pubName8), strcat(priName1, priName8);
            createRSAKeys(size, pubName1, pubName8, priName1, priName8);
        }
    }
    else
        printf("\nERROR - Invalid RSA Key Size\n");

    system("pause");
}