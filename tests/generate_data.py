# generated via chatGPT

import random
import string

def random_alphanum_string(max_length=16):
    length = random.randint(1, max_length)
    return ''.join(random.choices(string.ascii_letters + string.digits, k=length))

def random_anychar_string(max_length=16):
    allowed_chars = string.ascii_letters + string.digits + "!@#$%^&*()_+-=~,.<>?/\\|[]{}:;"
    length = random.randint(1, max_length)
    return ''.join(random.choices(allowed_chars, k=length))

def random_email():
    user = ''.join(random.choices(string.ascii_lowercase + string.digits, k=random.randint(5, 10)))
    domain = ''.join(random.choices(string.ascii_lowercase, k=random.randint(5, 8)))
    tld = random.choice(['com', 'net', 'org', 'io', 'co'])
    return f"{user}@{domain}.{tld}"

def generate_row():
    include_first = random.choice([True, False])
    include_second = random.choice([True, False])
    include_fourth = random.choice([True, False])

    first = random_alphanum_string() if include_first else ""
    second = random_anychar_string() if include_second else ""
    email = random_email()
    fourth = str(random.randint(-100, 100)) if include_fourth else ""

    # Build the row with space-separated columns, preserving empty fields
    columns = [first, second, email]
    if fourth or include_fourth:
        columns.append(fourth)
    return ' '.join(col if col else '' for col in columns).ljust(4 - len(columns) + len(' '.join(columns)))

# Generate and print 100 rows
for _ in range(100):
    print(generate_row())
