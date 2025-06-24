#include "gtest/gtest.h"
#include "type_safe_factory.hpp"
#include "data_packet.hpp" // Included for DataPacket
#include <memory>
#include <string>

// --- Test Fixture and Helper Classes ---

// Base class for factory products
class Animal {
public:
    virtual ~Animal() = default;
    virtual std::string speak() const = 0;
    virtual std::string name() const { return animal_name; }
    explicit Animal(std::string n) : animal_name(std::move(n)) {}
protected:
    std::string animal_name;
};

// Derived class 1
class Dog : public Animal {
public:
    explicit Dog(const common_utils::DataPacket& params) : Animal(params.getParam<std::string>("name")) {}
    std::string speak() const override { return "Woof"; }

    static std::shared_ptr<Animal> create(const common_utils::DataPacket& params) {
        return std::make_shared<Dog>(params);
    }
};

// Derived class 2
class Cat : public Animal {
public:
    explicit Cat(const common_utils::DataPacket& params) : Animal(params.getParam<std::string>("name")) {
        if (params.getOptionalParam<bool>("is_grumpy").value_or(false)) {
            grumpy_sound = "Hiss";
        }
    }
    std::string speak() const override { return grumpy_sound; }
     static std::shared_ptr<Animal> create(const common_utils::DataPacket& params) {
        return std::make_shared<Cat>(params);
    }
private:
    std::string grumpy_sound = "Meow";
};

// Derived class that might throw during construction via creator
class Alligator : public Animal {
public:
    explicit Alligator(const common_utils::DataPacket& params) : Animal("Alli") {
        if (params.getParam<bool>("should_throw")) {
            throw std::runtime_error("Alligator construction failed as requested");
        }
    }
    std::string speak() const override { return "Snap"; }
    static std::shared_ptr<Animal> create(const common_utils::DataPacket& params) {
        return std::make_shared<Alligator>(params);
    }
};


// Test fixture for Factory tests
class TypeSafeFactoryTest : public ::testing::Test {
protected:
    // Get a reference to the singleton factory instance for Animal
    common_utils::Factory<Animal>& animal_factory = common_utils::Factory<Animal>::instance();

    // Clean up any registered creators after each test to ensure test isolation
    // This is tricky with a singleton factory if we can't clear its internal map.
    // For a robust test suite, the Factory might need a reset/clear method for testing.
    // Assuming no such method, tests need to be careful about shared state.
    // One approach for some isolation: use different class names or base types per test if possible,
    // or accept that registration is global for the test run.
    // For now, we'll assume independent registrations or careful naming.
    // A better way would be a test-specific factory or a reset method.
    // Let's try to use unique names for each test registration where it matters.
};

TEST_F(TypeSafeFactoryTest, RegisterAndCreate) {
    bool registered = animal_factory.registerCreator("Dog", Dog::create);
    ASSERT_TRUE(registered);
    EXPECT_TRUE(animal_factory.isRegistered("Dog"));

    common_utils::DataPacket params;
    params.setParam<std::string>("name", "Buddy");

    std::shared_ptr<Animal> dog = animal_factory.create("Dog", params);
    ASSERT_NE(dog, nullptr);
    EXPECT_EQ(dog->speak(), "Woof");
    EXPECT_EQ(dog->name(), "Buddy");
}

TEST_F(TypeSafeFactoryTest, CreateUnregistered) {
    EXPECT_FALSE(animal_factory.isRegistered("Unicorn"));
    common_utils::DataPacket params;
    EXPECT_THROW(animal_factory.create("Unicorn", params), std::runtime_error);
}

TEST_F(TypeSafeFactoryTest, RegisterMultipleAndCreate) {
    ASSERT_TRUE(animal_factory.registerCreator("TestCat1", Cat::create));
    ASSERT_TRUE(animal_factory.registerCreator("TestDog1", Dog::create));

    common_utils::DataPacket cat_params;
    cat_params.setParam<std::string>("name", "Whiskers");
    std::shared_ptr<Animal> cat = animal_factory.create("TestCat1", cat_params);
    ASSERT_NE(cat, nullptr);
    EXPECT_EQ(cat->speak(), "Meow");
    EXPECT_EQ(cat->name(), "Whiskers");

    common_utils::DataPacket dog_params;
    dog_params.setParam<std::string>("name", "Rex");
    std::shared_ptr<Animal> dog = animal_factory.create("TestDog1", dog_params);
    ASSERT_NE(dog, nullptr);
    EXPECT_EQ(dog->speak(), "Woof");
    EXPECT_EQ(dog->name(), "Rex");
}

TEST_F(TypeSafeFactoryTest, RegisterExistingFails) {
    ASSERT_TRUE(animal_factory.registerCreator("DuplicateDog", Dog::create));
    // Attempting to register the same class name again should fail (return false)
    bool registered_again = animal_factory.registerCreator("DuplicateDog", Dog::create);
    EXPECT_FALSE(registered_again);
}

TEST_F(TypeSafeFactoryTest, CreateWithParams) {
    ASSERT_TRUE(animal_factory.registerCreator("GrumpyCat", Cat::create));

    common_utils::DataPacket params_normal;
    params_normal.setParam<std::string>("name", "Milo");
    std::shared_ptr<Animal> normal_cat = animal_factory.create("GrumpyCat", params_normal);
    ASSERT_NE(normal_cat, nullptr);
    EXPECT_EQ(normal_cat->speak(), "Meow");

    common_utils::DataPacket params_grumpy;
    params_grumpy.setParam<std::string>("name", "Oscar");
    params_grumpy.setParam<bool>("is_grumpy", true);
    std::shared_ptr<Animal> grumpy_cat = animal_factory.create("GrumpyCat", params_grumpy);
    ASSERT_NE(grumpy_cat, nullptr);
    EXPECT_EQ(grumpy_cat->speak(), "Hiss");
}

TEST_F(TypeSafeFactoryTest, RegisterNullCreatorThrows) {
    common_utils::Factory<Animal>::Creator null_creator = nullptr;
    EXPECT_THROW(animal_factory.registerCreator("NullCreatr", null_creator), std::runtime_error);
}

TEST_F(TypeSafeFactoryTest, CreatorThrowsException) {
    ASSERT_TRUE(animal_factory.registerCreator("TroubleAlligator", Alligator::create));

    common_utils::DataPacket params_ok;
    params_ok.setParam<bool>("should_throw", false);
    std::shared_ptr<Animal> alli_ok = animal_factory.create("TroubleAlligator", params_ok);
    ASSERT_NE(alli_ok, nullptr);
    EXPECT_EQ(alli_ok->speak(), "Snap");

    common_utils::DataPacket params_throw;
    params_throw.setParam<bool>("should_throw", true);
    // The factory should catch the exception from creator and rethrow its own
    EXPECT_THROW(animal_factory.create("TroubleAlligator", params_throw), std::runtime_error);
    try {
        animal_factory.create("TroubleAlligator", params_throw);
    } catch (const std::runtime_error& e) {
        std::string error_msg = e.what();
        EXPECT_NE(error_msg.find("Factory error: Failed to create 'TroubleAlligator'"), std::string::npos);
        EXPECT_NE(error_msg.find("Alligator construction failed as requested"), std::string::npos);
    }
}

// Test with a different base class to ensure template specialization works
class Vehicle {
public:
    virtual ~Vehicle() = default;
    virtual int num_wheels() const = 0;
};
class Car : public Vehicle {
public:
    Car(const common_utils::DataPacket&) {} // Unused params for this simple example
    int num_wheels() const override { return 4; }
    static std::shared_ptr<Vehicle> create(const common_utils::DataPacket& params) {
        return std::make_shared<Car>(params);
    }
};

TEST_F(TypeSafeFactoryTest, DifferentBaseClass) {
    common_utils::Factory<Vehicle>& vehicle_factory = common_utils::Factory<Vehicle>::instance();
    ASSERT_TRUE(vehicle_factory.registerCreator("MyCar", Car::create));

    common_utils::DataPacket params; // Empty params
    std::shared_ptr<Vehicle> car = vehicle_factory.create("MyCar", params);
    ASSERT_NE(car, nullptr);
    EXPECT_EQ(car->num_wheels(), 4);

    // Ensure animal_factory is distinct and doesn't know about "MyCar"
    EXPECT_FALSE(animal_factory.isRegistered("MyCar"));
    EXPECT_THROW(animal_factory.create("MyCar"), std::runtime_error);
}
