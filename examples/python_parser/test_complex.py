# A complex test
def factorial(n):
    """Compute factorial"""
    if n <= 1:
        return 1
    result = 1
    for i in range(2, n + 1):
        result *= i
    return result


x = 42  # the answer
y = 3.14
z = 0xFF + 0o77
print(f"x={x}, y={y}, z={z}")
