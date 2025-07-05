from typing import Final

import numpy as np
from numpy.typing import NDArray
from scipy.stats import binom, norm

## Statistics parameters, can be tuned for better performance
# 1 - α, or the likelihood that a Type-I error does not occur
CONFIDENCE: Final = 0.95
# 1 - β, or the likelihood that a Type-II error does not occur
POWER: Final = 0.93

## Pre-defined constants
# Number of rounds in each Rock, Paper, Scissors game
ROUNDS: Final = 20
# Expected gap between the correct choice and competitors
DELTA: Final = 1
# The probability of winning in a single round
PROB: Final = 1 / 3


def sample_size(start: NDArray[np.uint8]) -> NDArray[np.uint32]:
    """
    Calculate the sample size needed for a given start position.
    """
    alpha = (1 - CONFIDENCE) / 2
    z1a = norm.ppf(1 - alpha)
    z1b = norm.ppf(POWER)
    sigma2 = PROB * (1 - PROB)

    sn = 2 * (z1a + z1b) ** 2 * sigma2 / DELTA**2
    n = np.ceil((ROUNDS - start - 1) * sn).astype(np.uint32)
    return np.maximum(n, 1)


@np.vectorize
def p_correct_pick(s: np.uint8, n: np.uint32) -> np.float64:
    """
    Calculate the probability of making a correct pick given the sample sizes.
    """
    t = (ROUNDS - s - 1) * n

    def pk(k: int) -> float:
        return binom.pmf(k, t, PROB) * binom.cdf(k + n, t, PROB) ** 2

    p = sum(pk(k) for k in range(t + 1))
    return max(0, min(p, 1))


def show(name: str, n: int, p: float) -> None:
    """
    Display the results in a formatted manner.
    """
    print(f'{name:>6s}, n = {n:4d}, 3n = {3 * n:4d}, p = {p:.9f}, 1-p = {1 - p:.9f}')


if __name__ == '__main__':
    print('Rock, Paper, Scissors - Probability Analysis')
    print('--------------------------------------------------')
    print(f'Confidence: {CONFIDENCE:.2f}, Power: {POWER:.2f}')

    s = np.arange(ROUNDS, dtype=np.uint8)
    n = sample_size(s)
    p = p_correct_pick(s, n)

    for i in s:
        show(f's = {i:2d}', n[i], p[i])
    show('total', np.sum(n), np.prod(p))
