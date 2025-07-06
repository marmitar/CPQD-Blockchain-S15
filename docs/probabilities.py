from typing import Final

import numpy as np
from numpy.typing import NDArray
from scipy.stats import binom, norm

## Statistics parameters, can be tuned for better performance
# 1 - α, or the likelihood that a Type-I error does not occur
CONFIDENCE: Final = 0.95
# 1 - β, or the likelihood that a Type-II error does not occur
POWER: Final = 0.90

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
    alpha = (1 - CONFIDENCE) / 3
    z1a = norm.ppf(1 - alpha)
    z1b = norm.ppf(POWER)
    sigma2 = PROB * (1 - PROB)

    sn = 2 * (z1a + z1b) ** 2 * sigma2 / DELTA**2
    n = np.ceil((ROUNDS - start - 1) * sn).astype(np.uint32)
    return np.maximum(n, 1)


def p_correct_pick(s: np.uint8, n: np.uint32) -> np.float64:
    """
    Calculate the probability of making a correct pick given the sample sizes.
    """
    t = (ROUNDS - s - 1) * n

    def p_k(k: int) -> float:
        """Probability that it picks the correct value, given that it wins `k` times."""

        # probability that the correct value wins `k` random rounds in total
        p_correct = binom.pmf(k, t, PROB)
        # probability that a wrong value wins less than `k + n` random rounds in total
        # if the correct value is 0, the wrong value can win up to `k + n` rounds inclusive,
        # because we favor 0 on draws
        p_wrong = binom.cdf(k + n - 1, t, PROB) + binom.pmf(k + n, t, PROB) / 3
        # since the correct value won `k` random and `n` fixed (by virtue of being correct),
        # and the wrong ones won less then `k + n` (random only), then we choose the correct value
        return p_correct * p_wrong**2

    p = sum(p_k(k) for k in range(t + 1))
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
    p = []

    for i in s:
        pi = p_correct_pick(s[i], n[i])
        show(f's = {i:2d}', n[i], pi)
        p.append(pi)

    show('total', np.sum(n), np.prod(p))
